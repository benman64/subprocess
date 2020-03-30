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

#endif

#include <string.h>
#include <thread>
#include <mutex>

#include "shell_utils.hpp"
#include "environ.hpp"

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


    Popen::Popen(CommandLine command, const PopenOptions& options) {
        ProcessBuilder builder;

        builder.cin_option  = options.cin;
        builder.cout_option = options.cout;
        builder.cerr_option = options.cerr;

        builder.env = options.env;
        builder.cwd = options.cwd;

        *this = builder.run_command(command);
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
        pid = 0;
        returncode = -1000;
        args.clear();
#ifdef _WIN32
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
#endif
    }
#ifdef _WIN32
    int Popen::wait(double timeout) {
        WaitForSingleObject(process_info.hProcess, INFINITE );
        DWORD exit_code;
        GetExitCodeProcess(process_info.hProcess, &exit_code);
        returncode = exit_code;
        return returncode;
    }
    // TODO: poll
    bool Popen::send_signal(int signum) {
        if (signum == SIGKILL) {
            return TerminateProcess(pid, 1);
        } else if (signum == SIGINT) {
            // can I use pid for processgroupid?
            return GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid);
        } else if (signum == SIGTERM) {
            return GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
        }
        return GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
    }
#else
    int Popen::wait(double timeout) {
        // TODO: timeout
        int exit_code;
        if (cout != kBadPipeValue)
            pipe_close(cout);
        if (cerr != kBadPipeValue)
            pipe_close(cerr);
        waitpid(pid, &exit_code,0);
        returncode = exit_code;
        return returncode;
    }

    bool Popen::send_signal(int signum) {
        return ::kill(pid, signum) == 0;
    }
#endif
    bool Popen::terminate() {
        return send_signal(SIGTERM);
    }
    bool Popen::kill() {
        return send_signal(SIGKILL);
    }










    std::string ProcessBuilder::windows_command() {
        return this->command[0];
    }

    std::string ProcessBuilder::windows_args() {
        return this->windows_args(this->command);
    }
    std::string ProcessBuilder::windows_args(const CommandLine& command) {
        std::string args;
        for(unsigned int i = 1; i < command.size(); ++i) {
            if (i > 1)
                args += ' ';
            args += escape_shell_arg(command[i]);
        }
        return args;
    }

#ifdef _WIN32

#else
    Popen ProcessBuilder::run_command(const CommandLine& command) {
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
            if(posix_spawn(&pid, args[0], &action, NULL, &args[0], env) != 0)
                throw SpawnError("posix_spawn failed with error: " + std::string(strerror(errno)));
        }
        args.clear();
        env_store.clear();

        if (cout_pair)
            pipe_close(cout_pair.output);
        if (cerr_pair)
            pipe_close(cerr_pair.output);

        process.pid = pid;
        process.args = CommandLine(command.begin()+1, command.end());
        return process;
    }
#endif


    std::string read_all(PipeHandle handle) {
        if (handle == kBadPipeValue)
            return {};
        constexpr int buf_size = 2048;
        uint8_t buf[buf_size];
        std::string result;
        while(true) {
            size_t transfered = pipe_read(handle, buf, buf_size);
            if(transfered > 0) {
                result.insert(result.end(), &buf[0], &buf[transfered]);
            } else {
                break;
            }
        }
        return result;
    }

    CompletedProcess run(CommandLine command, PopenOptions options) {
        Popen popen(command, options);

        CompletedProcess completed;
        std::thread cout_thread;
        std::thread cerr_thread;
        if (popen.cout != kBadPipeValue) {
            cout_thread = std::thread([&]() {
                try {
                    completed.cout = read_all(popen.cout);
                } catch (...) {
                }
                pipe_close(popen.cout);
                popen.cout = kBadPipeValue;
            });
        }
        if (popen.cerr != kBadPipeValue) {
            cerr_thread = std::thread([&]() {
                try {
                    completed.cerr = read_all(popen.cerr);
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
        completed.args = CommandLine(command.begin()+1, command.end());
        return completed;
    }
}