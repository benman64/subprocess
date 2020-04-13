#ifndef _WIN32
#include "ProcessBuilder.hpp"

#include <spawn.h>
#include <cstring>
#include <mutex>
#include <errno.h>

#include "environ.hpp"

extern "C" char **environ;

using std::nullptr_t;

using namespace subprocess::details;

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
    struct FileActions {
        FileActions() {
            int result = posix_spawn_file_actions_init(&actions);
            throw_os_error("posix_spawn_file_actions_init", result);
        }
        ~FileActions() {
            int result = posix_spawn_file_actions_destroy(&actions);
            throw_os_error("posix_spawn_file_actions_destroy", result);
        }

        void adddup2(int fd, int newfd) {
            int result = posix_spawn_file_actions_adddup2(&actions, fd, newfd);
            throw_os_error("posix_spawn_file_actions_adddup2", result);
        }
        void addclose(int fd) {
            int result = posix_spawn_file_actions_addclose(&actions, fd);
            throw_os_error("posix_spawn_file_actions_addclose", result);
        }

        posix_spawn_file_actions_t* get() {return &actions;}
        posix_spawn_file_actions_t actions;
    };

#ifndef _WIN32
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

        FileActions actions;

        if (cin_option == PipeOption::close)
            actions.addclose(kStdInValue);
        else if (cin_option == PipeOption::specific) {
            if (this->cin_pipe == kBadPipeValue) {
                throw std::invalid_argument("ProcessBuilder: bad pipe value for cin");
            }

            pipe_set_inheritable(this->cin_pipe, true);
            actions.adddup2(this->cin_pipe, kStdInValue);
            actions.addclose(this->cin_pipe);
        } else if (cin_option == PipeOption::pipe) {
            cin_pair = pipe_create();
            actions.addclose(cin_pair.output);
            actions.adddup2(cin_pair.input, kStdInValue);
            actions.addclose(cin_pair.input);
            process.cin = cin_pair.output;
        }


        if (cout_option == PipeOption::close)
            actions.addclose(kStdOutValue);
        else if (cout_option == PipeOption::pipe) {
            cout_pair = pipe_create();
            actions.addclose(cout_pair.input);
            actions.adddup2(cout_pair.output, kStdOutValue);
            actions.addclose(cout_pair.output);
            process.cout = cout_pair.input;
        } else if (cout_option == PipeOption::cerr) {
            // we have to wait until stderr is setup first
        } else if (cout_option == PipeOption::specific) {
            if (this->cout_pipe == kBadPipeValue) {
                throw std::invalid_argument("ProcessBuilder: bad pipe value for cout");
            }
            pipe_set_inheritable(this->cout_pipe, true);
            actions.adddup2(this->cout_pipe, kStdOutValue);
            actions.addclose(this->cout_pipe);
        }

        if (cerr_option == PipeOption::close)
            actions.addclose(kStdErrValue);
        else if (cerr_option == PipeOption::pipe) {
            cerr_pair = pipe_create();
            actions.addclose(cerr_pair.input);
            actions.adddup2(cerr_pair.output, kStdErrValue);
            actions.addclose(cerr_pair.output);
            process.cerr = cerr_pair.input;
        } else if (cerr_option == PipeOption::cout) {
            actions.adddup2(kStdOutValue, kStdErrValue);
        } else if (cerr_option == PipeOption::specific) {
            if (this->cerr_pipe == kBadPipeValue) {
                throw std::invalid_argument("ProcessBuilder: bad pipe value for cerr");
            }

            pipe_set_inheritable(this->cerr_pipe, true);
            actions.adddup2(this->cerr_pipe, kStdErrValue);
            actions.addclose(this->cerr_pipe);
        }

        if (cout_option == PipeOption::cerr) {
            actions.adddup2(kStdErrValue, kStdOutValue);
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
        struct SpawnAttr {
            SpawnAttr(posix_spawnattr_t& attributes) {
                this->attributes = &attributes;
            }
            ~SpawnAttr() {
                int ret = posix_spawnattr_destroy(attributes);
                throw_os_error("posix_spawnattr_destroy", ret);
            }

            void setflags(short flags) {
                int ret = posix_spawnattr_setflags(attributes, flags);
                throw_os_error("posix_spawnattr_setflags", ret);
            }
            posix_spawnattr_t* attributes;
        } attributes_raii(attributes);
#if 0
        // I can't think of a nice way to make this configurable.
        posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGMASK);
        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        posix_spawnattr_setsigmask(&attributes, &signal_mask);
#endif
        int flags = this->new_process_group? POSIX_SPAWN_SETSIGMASK : 0;
#ifdef POSIX_SPAWN_USEVFORK
        flags |= POSIX_SPAWN_USEVFORK;
#endif
        attributes_raii.setflags(flags);
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
            int ret = posix_spawn(&pid, args[0], actions.get(), &attributes, &args[0], env);
            if(ret != 0)
                throw SpawnError("posix_spawn failed with error: " + std::string(strerror(ret)));
        }
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
}
#endif