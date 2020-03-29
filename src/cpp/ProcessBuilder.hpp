#pragma once

#include <vector>
#include <string>

#include "pipe.hpp"

namespace subprocess {

    /*  For reference:
        0 - cin
        1 - cout
        2 - cerr
    */

    struct PopenOptions {
        bool        check   = false;
        PipeOption  cin     = PipeOption::inherit;
        PipeOption  cout    = PipeOption::inherit;
        PipeOption  cerr    = PipeOption::inherit;

        std::string cwd;
        /** If empty inherits from current process */
        EnvMap      env;

        /** Timeout in seconds. Raise TimeoutExpired.

            Only available if you use subprocess_run
        */
        double timeout  = -1;
    };
    struct PopenBuilder {
        PopenOptions options;

        PopenBuilder& check(bool ch) {options.check = ch; return *this;}
        PopenBuilder& cin(PipeOption cin) {options.cin = cin; return *this;}
        PopenBuilder& cout(PipeOption cout) {options.cout = cout; return *this;}
        PopenBuilder& cerr(PipeOption cerr) {options.cerr = cerr; return *this;}
        PopenBuilder& cwd(std::string cwd) {options.cwd = cwd; return *this;}
        PopenBuilder& env(EnvMap& env) {options.env = env; return *this;}
        PopenBuilder& timeout(double timeout) {options.timeout = timeout; return *this;}

        PopenOptions& build() {return options;}
    };
    class ProcessBuilder;
    /** Active running process.

        Similar design of subprocess.Popen. In c++ I didn't like
    */
    struct Popen {
    public:
        Popen(){}
        Popen(const PopenOptions& options);
        Popen(const Popen&)=delete;
        Popen& operator=(const Popen&)=delete;

        Popen(Popen&&);
        Popen& operator=(Popen&&);

        /** Waits for process, Closes pipes and destroys any handles.*/
        ~Popen();

        /** Write to this stream to send data to the process */
        PipeHandle  cin       = kBadPipeValue;
        /** Read from this stream to get output of process */
        PipeHandle  cout      = kBadPipeValue;
        /** Read from this stream to get cerr output of process */
        PipeHandle  cerr      = kBadPipeValue;


        pid_t       pid         = 0;
        int         returncode  = -1000;
        CommandLine args;

        /** @return true if terminated. */
        bool poll();
        /**
            @param timeout  timeout in seconds. Raises TimeoutExpired on
                            timeout. NOT IMPLEMENTED, WILL WAIT FOREVER.
            @return returncode
        */
        int wait(double timeout=0);
        /** Send the signal to the process.

            On windows SIGTERM is an alias for terminate()
        */
        void send_signal(int signal);
        /** on windows calls TerminateProcess() */
        void terminate();
        /** sends SIGKILL on linux, alias for terminate() on windows.
        */
        void kill();

        friend ProcessBuilder;
    private:
#ifdef _WIN32
        PROCESS_INFORMATION process_info;
#endif
    };

    class ProcessBuilder {
    public:
        std::vector<PipeHandle> child_close_pipes;

        PipeHandle cin_pipe       = kBadPipeValue;
        PipeHandle cout_pipe      = kBadPipeValue;
        PipeHandle cerr_pipe      = kBadPipeValue;


        PipeOption cin_option     = PipeOption::inherit;
        PipeOption cout_option    = PipeOption::inherit;
        PipeOption cerr_option    = PipeOption::inherit;

        /** If empty inherits from current process */
        EnvMap      env;
        std::string cwd;
        CommandLine command;

        std::string windows_command();
        std::string windows_args();

        Popen run() {
            return run_command(this->command);
        }
        Popen run_command(const CommandLine& command);
    };


    CompletedProcess run(CommandLine command, PopenOptions options);
}