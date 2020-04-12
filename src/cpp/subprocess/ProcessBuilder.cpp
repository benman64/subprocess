#include "ProcessBuilder.hpp"

#ifdef _WIN32

#else
#include <spawn.h>
#ifdef __APPLE__
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#include <errno.h>
#include <signal.h>
#endif

#include <string.h>
#include <thread>
#include <mutex>
#include <chrono>

#include "shell_utils.hpp"
#include "environ.hpp"
#include "utf8_to_utf16.hpp"

extern "C" char **environ;

using std::nullptr_t;
namespace {
    struct cstring_vector {
        typedef char* value_type;
        ~cstring_vector() {
            clear();
        }
        void clear() {
            for (char* ptr : m_list) {
                if (ptr)
                    free(ptr);
            }
            m_list.clear();
        }

        void reserve(std::size_t size) {
            m_list.reserve(size);
        }
        void push_back(const std::string& str) {
            push_back(str.c_str());
        }
        void push_back(nullptr_t) {
            m_list.push_back(nullptr);
        }
        void push_back(const char* str) {
            char* copy = str? strdup(str) : nullptr;
            m_list.push_back(copy);
        }
        void push_back_owned(char* str) {
            m_list.push_back(str);
        }
        value_type& operator[](std::size_t index) {
            return m_list[index];
        }
        char** data() {
            return &m_list[0];
        }
        std::vector<char*> m_list;
    };

}
namespace subprocess {
    double monotonic_seconds() {
        static bool needs_init = true;
        static std::chrono::steady_clock::time_point begin;
        static double last_value = 0;
        if (needs_init) {
            begin = std::chrono::steady_clock::now();
            needs_init = false;
        }
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = now - begin;
        double result = duration.count();

        // some OS's have bugs and not exactly monotonic. Or perhaps there is
        // floating point errors or something. I don't know.
        if (result < last_value)
            return last_value;
        last_value = result;
        return result;
    }

    double sleep_seconds(double seconds) {
        StopWatch watch;
        std::chrono::duration<double> duration(seconds);
        std::this_thread::sleep_for(duration);
        return watch.seconds();
    }

    struct AutoClosePipe {
        AutoClosePipe(PipeHandle handle, bool autoclose) {
            mHandle = autoclose? handle : kBadPipeValue;
        }
        ~AutoClosePipe() {
            close();
        }
        void close() {
            if (mHandle != kBadPipeValue) {
                pipe_close(mHandle);
                mHandle = kBadPipeValue;
            }
        }
    private:
        PipeHandle mHandle;
    };
    void pipe_thread(PipeHandle input, std::ostream* output) {
        std::thread thread([=]() {
            std::vector<char> buffer(2048);
            while (true) {
                ssize_t transfered = pipe_read(input, &buffer[0], buffer.size());
                if (transfered <= 0)
                    break;
                output->write(&buffer[0], transfered);
            }
        });
        thread.detach();
    }

    void pipe_thread(PipeHandle input, FILE* output) {
        std::thread thread([=]() {
            std::vector<char> buffer(2048);
            while (true) {
                ssize_t transfered = pipe_read(input, &buffer[0], buffer.size());
                if (transfered <= 0)
                    break;
                fwrite(&buffer[0], 1, transfered, output);
            }
        });
        thread.detach();
    }
    void pipe_thread(FILE* input, PipeHandle output, bool bautoclose) {
        std::thread thread([=]() {
            AutoClosePipe autoclose(output, bautoclose);
            std::vector<char> buffer(2048);
            while (true) {
                ssize_t transfered = fread(&buffer[0], 1, buffer.size(), input);
                if (transfered <= 0)
                    break;
                pipe_write(output, &buffer[0], transfered);
            }
        });
        thread.detach();
    }
    void pipe_thread(std::string& input, PipeHandle output, bool bautoclose) {
        std::thread thread([input(move(input)), output, bautoclose]() {
            AutoClosePipe autoclose(output, bautoclose);

            std::size_t pos = 0;
            while (pos < input.size()) {
                ssize_t transfered = pipe_write(output, input.c_str()+pos, input.size() - pos);
                if (transfered <= 0)
                    break;
                pos += transfered;
            }
        });
        thread.detach();
    }
    void pipe_thread(std::istream* input, PipeHandle output, bool bautoclose) {
        std::thread thread([=]() {
            AutoClosePipe autoclose(output, bautoclose);
            std::vector<char> buffer(2048);
            while (true) {
                input->read(&buffer[0], buffer.size());
                ssize_t transfered = input->gcount();
                if (input->bad())
                    break;
                if (transfered <= 0) {
                    if (input->eof())
                        break;
                    continue;
                }
                pipe_write(output, &buffer[0], transfered);
            }
        });
        thread.detach();
    }
    void setup_redirect_stream(PipeHandle input, PipeVar& output) {
        PipeVarIndex index = static_cast<PipeVarIndex>(output.index());

        switch (index) {
        case PipeVarIndex::handle:
        case PipeVarIndex::option: return;
        case PipeVarIndex::string: // doesn't make sense
        case PipeVarIndex::istream: // dousn't make sense
            throw std::runtime_error("expected something to output to");
        case PipeVarIndex::ostream:
            pipe_thread(input, std::get<std::ostream*>(output));
            break;
        case PipeVarIndex::file:
            pipe_thread(input, std::get<FILE*>(output));
            break;
        }
    }

    void setup_redirect_stream(PipeVar& input, PipeHandle output) {
        PipeVarIndex index = static_cast<PipeVarIndex>(input.index());

        switch (index) {
        case PipeVarIndex::handle:
        case PipeVarIndex::option: return;
        case PipeVarIndex::string:
            pipe_thread(std::get<std::string>(input), output, true);
            break;
        case PipeVarIndex::istream:
            pipe_thread(std::get<std::istream*>(input), output, true);
            break;
        case PipeVarIndex::ostream:
            throw std::runtime_error("reading from std::ostream doesn't make sense");
        case PipeVarIndex::file:
            pipe_thread(std::get<FILE*>(input), output, true);
            break;
        }
    }
    Popen::Popen(CommandLine command, const RunOptions& optionsIn) {
        // we have to make a copy because of const
        RunOptions options = optionsIn;
        init(command, options);
    }

    Popen::Popen(CommandLine command, RunOptions&& optionsIn) {
        RunOptions options = std::move(optionsIn);
        init(command, options);
    }
    void Popen::init(CommandLine& command, RunOptions& options) {
        ProcessBuilder builder;

        builder.cin_option  = get_pipe_option(options.cin);
        builder.cout_option = get_pipe_option(options.cout);
        builder.cerr_option = get_pipe_option(options.cerr);

        if (builder.cin_option == PipeOption::specific) {
            builder.cin_pipe = std::get<PipeHandle>(options.cin);
            if (builder.cin_pipe == kBadPipeValue)
                throw std::runtime_error("bad pipe value for cin");
        }
        if (builder.cout_option == PipeOption::specific) {
            builder.cout_pipe = std::get<PipeHandle>(options.cout);
            if (builder.cout_pipe == kBadPipeValue)
                throw std::runtime_error("Popen constructor: bad pipe value for cout");
        }
        if (builder.cerr_option == PipeOption::specific) {
            builder.cerr_pipe = std::get<PipeHandle>(options.cerr);
            if (builder.cout_pipe == kBadPipeValue)
                throw std::runtime_error("Popen constructor: bad pipe value for cout");
        }
        builder.env = options.env;
        builder.cwd = options.cwd;

        *this = builder.run_command(command);

        setup_redirect_stream(options.cin, cin);
        setup_redirect_stream(cout, options.cout);
        setup_redirect_stream(cerr, options.cerr);
    }

    Popen::Popen(Popen&& other) {
        *this = std::move(other);
    }

    Popen& Popen::operator=(Popen&& other) {
        close();
        cin = other.cin;
        cout = other.cout;
        cerr = other.cerr;

        pid = other.pid;
        returncode = other.returncode;
        args = std::move(other.args);

#ifdef _WIN32
        process_info = other.process_info;
        other.process_info = {0};
#endif

        other.cin = kBadPipeValue;
        other.cout = kBadPipeValue;
        other.cerr = kBadPipeValue;
        other.pid = 0;
        other.returncode = -1000;
        return *this;
    }

    Popen::~Popen() {
        close();
    }
    void Popen::close() {
        if (cin != kBadPipeValue)
            pipe_close(cin);
        if (cout != kBadPipeValue)
            pipe_close(cout);
        if (cerr != kBadPipeValue)
            pipe_close(cerr);
        cin = cout = cerr = kBadPipeValue;

        // do this to not have zombie processes.
        if (pid) {
            wait();

#ifdef _WIN32
            CloseHandle(process_info.hProcess);
            CloseHandle(process_info.hThread);
#endif
        }
        pid = 0;
        returncode = kBadReturnCode;
        args.clear();
    }
#ifdef _WIN32
    std::string lastErrorString() {
        LPTSTR lpMsgBuf = nullptr;
        DWORD dw        = GetLastError();

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        std::string message = lptstr_to_string((LPTSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
        return message;
    }
    bool Popen::poll() {
        if (returncode != kBadReturnCode)
            return true;
        DWORD ms = 0;
        DWORD result = WaitForSingleObject(process_info.hProcess, ms);
        if (result == WAIT_TIMEOUT) {
            return false;
        } else if (result == WAIT_ABANDONED) {
            DWORD error = GetLastError();
            throw OSError("WAIT_ABANDONED error:" + std::to_string(error));
        } else if (result == WAIT_FAILED) {
            DWORD error = GetLastError();
            throw OSError("WAIT_FAILED error:" + std::to_string(error) + ":" + lastErrorString());
        }
        if (result != WAIT_OBJECT_0) {
            throw OSError("WaitForSingleObject failed: " + std::to_string(result));
        }
        DWORD exit_code;
        GetExitCodeProcess(process_info.hProcess, &exit_code);
        returncode = exit_code;
        return true;
    }

    int Popen::wait(double timeout) {
        if (returncode != kBadReturnCode)
            return returncode;
        DWORD ms = timeout < 0? INFINITE : timeout*1000.0;
        DWORD result = WaitForSingleObject(process_info.hProcess, ms);
        if (result == WAIT_TIMEOUT) {
            throw TimeoutExpired("timeout of " + std::to_string(ms) + " expired");
        } else if (result == WAIT_ABANDONED) {
            DWORD error = GetLastError();
            throw OSError("WAIT_ABANDONED error:" + std::to_string(error));
        } else if (result == WAIT_FAILED) {
            DWORD error = GetLastError();
            throw OSError("WAIT_FAILED error:" + std::to_string(error) + ":" + lastErrorString());
        }
        if (result != WAIT_OBJECT_0) {
            throw OSError("WaitForSingleObject failed: " + std::to_string(result));
        }
        DWORD exit_code;
        GetExitCodeProcess(process_info.hProcess, &exit_code);
        returncode = exit_code;
        return returncode;
    }

    bool Popen::send_signal(int signum) {
        if (returncode != kBadReturnCode)
            return false;
        bool success = false;
        if (signum == PSIGKILL || signum == PSIGTERM) {
            return TerminateProcess(process_info.hProcess, 1);
        } else if (signum == PSIGINT) {
            // can I use pid for processgroupid?
            success = GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid);
        } else if (signum == PSIGTERM) {
            success = GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
        } else {
            success = GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
        }
        return success;
    }
#else
    bool Popen::poll() {
        if (returncode != kBadReturnCode)
            return true;
        int exit_code;
        auto child = waitpid(pid, &exit_code, WNOHANG);
        if (child == 0)
            return false;
        if (child > 0)
            returncode = exit_code;
        return child > 0;
    }
    int Popen::wait(double timeout) {
        if (returncode != kBadReturnCode)
            return returncode;
        if (timeout < 0) {
            int exit_code;
            while (true) {
                pid_t child = waitpid(pid, &exit_code,0);
                if (child == -1 && errno == EINTR) {
                    continue;
                }
                if (child == -1) {
                    // TODO: throw oserror(errno)
                }
                break;
            }
            returncode = exit_code;
            return returncode;
        }
        StopWatch watch;

        while (watch.seconds() < timeout) {
            if (poll())
                return returncode;
            sleep_seconds(0.00001);
        }
        throw TimeoutExpired("no time");
    }

    bool Popen::send_signal(int signum) {
        if (returncode != kBadReturnCode)
            return false;
        return ::kill(pid, signum) == 0;
    }
#endif
    bool Popen::terminate() {
        return send_signal(PSIGTERM);
    }
    bool Popen::kill() {
        return send_signal(PSIGKILL);
    }










    std::string ProcessBuilder::windows_command() {
        return this->command[0];
    }

    std::string ProcessBuilder::windows_args() {
        return this->windows_args(this->command);
    }
    std::string ProcessBuilder::windows_args(const CommandLine& command) {
        std::string args;
        for(unsigned int i = 0; i < command.size(); ++i) {
            if (i > 0)
                args += ' ';
            args += escape_shell_arg(command[i]);
        }
        return args;
    }

#ifdef _WIN32

#else
    Popen ProcessBuilder::run_command(const CommandLine& command) {
        if (command.empty()) {
            throw std::invalid_argument("command should not be empty");
        }
        std::string program = find_program(command[0]);
        if(program.empty()) {
            throw CommandNotFoundError("command not found " + command[0]);
        }

        Popen process;
        PipePair cin_pair;
        PipePair cout_pair;
        PipePair cerr_pair;

        posix_spawn_file_actions_t action;

        posix_spawn_file_actions_init(&action);
        if (cin_option == PipeOption::close)
            posix_spawn_file_actions_addclose(&action, kStdInValue);
        else if (cin_option == PipeOption::specific) {
            if (this->cin_pipe == kBadPipeValue) {
                throw std::runtime_error("ProcessBuilder: bad pipe value for cin");
            }

            pipe_set_inheritable(this->cin_pipe, true);
            posix_spawn_file_actions_adddup2(&action, this->cin_pipe, kStdInValue);
            posix_spawn_file_actions_addclose(&action, this->cin_pipe);
        } else if (cin_option == PipeOption::pipe) {
            cin_pair = pipe_create();
            posix_spawn_file_actions_addclose(&action, cin_pair.output);
            posix_spawn_file_actions_adddup2(&action, cin_pair.input, kStdInValue);
            posix_spawn_file_actions_addclose(&action, cin_pair.input);
            process.cin = cin_pair.output;
        }


        if (cout_option == PipeOption::close)
            posix_spawn_file_actions_addclose(&action, kStdOutValue);
        else if (cout_option == PipeOption::pipe) {
            cout_pair = pipe_create();
            posix_spawn_file_actions_addclose(&action, cout_pair.input);
            posix_spawn_file_actions_adddup2(&action, cout_pair.output, kStdOutValue);
            posix_spawn_file_actions_addclose(&action, cout_pair.output);
            process.cout = cout_pair.input;
        } else if (cout_option == PipeOption::cerr) {
            // we have to wait until stderr is setup first
        } else if (cout_option == PipeOption::specific) {
            if (this->cout_pipe == kBadPipeValue) {
                throw std::runtime_error("ProcessBuilder: bad pipe value for cout");
            }
            pipe_set_inheritable(this->cout_pipe, true);
            posix_spawn_file_actions_adddup2(&action, this->cout_pipe, kStdOutValue);
            posix_spawn_file_actions_addclose(&action, this->cout_pipe);
        }

        if (cerr_option == PipeOption::close)
            posix_spawn_file_actions_addclose(&action, kStdErrValue);
        else if (cerr_option == PipeOption::pipe) {
            cerr_pair = pipe_create();
            posix_spawn_file_actions_addclose(&action, cerr_pair.input);
            posix_spawn_file_actions_adddup2(&action, cerr_pair.output, kStdErrValue);
            posix_spawn_file_actions_addclose(&action, cerr_pair.output);
            process.cerr = cerr_pair.input;
        } else if (cerr_option == PipeOption::cout) {
            posix_spawn_file_actions_adddup2(&action, kStdOutValue, kStdErrValue);
        } else if (cerr_option == PipeOption::specific) {
            if (this->cerr_pipe == kBadPipeValue) {
                throw std::runtime_error("ProcessBuilder: bad pipe value for cerr");
            }

            pipe_set_inheritable(this->cerr_pipe, true);
            posix_spawn_file_actions_adddup2(&action, this->cerr_pipe, kStdErrValue);
            posix_spawn_file_actions_addclose(&action, this->cerr_pipe);
        }

        if (cout_option == PipeOption::cerr) {
            posix_spawn_file_actions_adddup2(&action, kStdErrValue, kStdOutValue);
        }
        pid_t pid;
        cstring_vector args;
        args.reserve(command.size()+1);
        args.push_back(program);
        for(size_t i = 1; i < command.size(); ++i) {
            args.push_back(command[i]);
        }
        args.push_back(nullptr);
        char** env = environ;
        cstring_vector env_store;
        if (!this->env.empty()) {
            for (auto& pair : this->env) {
                std::string line = pair.first + "=" + pair.second;
                env_store.push_back(line);
            }
            env_store.push_back(nullptr);
            env = &env_store[0];
        }

        posix_spawnattr_t attributes;
        posix_spawnattr_init(&attributes);
#if 0
        // I can't think of a nice way to make this configurable.
        posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGMASK);
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        posix_spawnattr_setsigmask(&attributes, &signal_mask);
#endif
        int flags = 0;
#ifdef POSIX_SPAWN_USEVFORK
        flags |= POSIX_SPAWN_USEVFORK;
#endif
        posix_spawnattr_setflags(&attributes, flags);
        {
            /*  I should have gone with vfork() :(
                TODO: reimplement with vfork for thread safety.

                this locking solution should work in practice just fine.
            */
            static std::mutex mutex;
            std::unique_lock<std::mutex> lock(mutex);
            CwdGuard cwdGuard;
            if (!this->cwd.empty())
                subprocess::setcwd(this->cwd);
            if(posix_spawn(&pid, args[0], &action, &attributes, &args[0], env) != 0)
                throw SpawnError("posix_spawn failed with error: " + std::string(strerror(errno)));
        }
        posix_spawnattr_destroy(&attributes);
        posix_spawn_file_actions_destroy(&action);
        args.clear();
        env_store.clear();
        if (cin_pair)
            cin_pair.close_input();
        if (cout_pair)
            cout_pair.close_output();
        if (cerr_pair)
            cerr_pair.close_output();
        cin_pair.disown();
        cout_pair.disown();
        cerr_pair.disown();
        process.pid = pid;
        process.args = command;
        return process;
    }
#endif

    CompletedProcess run(Popen& popen, bool check) {
        CompletedProcess completed;
        std::thread cout_thread;
        std::thread cerr_thread;
        if (popen.cout != kBadPipeValue) {
            cout_thread = std::thread([&]() {
                try {
                    completed.cout = pipe_read_all(popen.cout);
                } catch (...) {
                }
                pipe_close(popen.cout);
                popen.cout = kBadPipeValue;
            });
        }
        if (popen.cerr != kBadPipeValue) {
            cerr_thread = std::thread([&]() {
                try {
                    completed.cerr = pipe_read_all(popen.cerr);
                } catch (...) {
                }
                pipe_close(popen.cerr);
                popen.cerr = kBadPipeValue;
            });
        }

        if (cout_thread.joinable()) {
            cout_thread.join();
        }
        if (cerr_thread.joinable()) {
            cerr_thread.join();
        }

        popen.wait();
        completed.returncode = popen.returncode;
        completed.args = CommandLine(popen.args.begin()+1, popen.args.end());
        if (check) {
            CalledProcessError error("failed to execute " + popen.args[0]);
            error.cmd           = popen.args;
            error.returncode    = completed.returncode;
            error.cout          = std::move(completed.cout);
            error.cerr          = std::move(completed.cerr);
            throw error;
        }
        return completed;
    }

    CompletedProcess run(CommandLine command, RunOptions options) {
        Popen popen(command, std::move(options));
        CompletedProcess completed;
        std::thread cout_thread;
        std::thread cerr_thread;
        if (popen.cout != kBadPipeValue) {
            cout_thread = std::thread([&]() {
                try {
                    completed.cout = pipe_read_all(popen.cout);
                } catch (...) {
                }
                pipe_close(popen.cout);
                popen.cout = kBadPipeValue;
            });
        }
        if (popen.cerr != kBadPipeValue) {
            cerr_thread = std::thread([&]() {
                try {
                    completed.cerr = pipe_read_all(popen.cerr);
                } catch (...) {
                }
                pipe_close(popen.cerr);
                popen.cerr = kBadPipeValue;
            });
        }

        if (cout_thread.joinable()) {
            cout_thread.join();
        }
        if (cerr_thread.joinable()) {
            cerr_thread.join();
        }

        popen.wait();
        completed.returncode = popen.returncode;
        completed.args = command;
        if (options.check) {
            CalledProcessError error("failed to execute " + command[0]);
            error.cmd           = command;
            error.returncode    = completed.returncode;
            error.cout          = std::move(completed.cout);
            error.cerr          = std::move(completed.cerr);
            throw error;
        }
        return completed;
    }

}