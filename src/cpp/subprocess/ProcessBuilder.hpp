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
    /*  possible names:
            PopenOptions,
            RunDef,
            RunConfig
            RunOptions
    */
    /*  New idea: Have 2 identical structs with only difference is cout
        default is with respect to running, or popen.
    */

    struct PopenOptions {
        bool        check   = false;
        PipeOption  cin     = PipeOption::inherit;
        PipeOption  cout    = PipeOption::pipe;
        PipeOption  cerr    = PipeOption::inherit;

        std::string cwd;
        /** If empty inherits from current process */
        EnvMap      env;

        /** Timeout in seconds. Raise TimeoutExpired.

            Only available if you use subprocess_run
        */
        double timeout  = -1;
    };

    struct RunOptions {
        RunOptions(){};
        RunOptions(const PopenOptions& options) {
            check   = options.check;
            cin     = options.cin;
            cout    = options.cout;
            cerr    = options.cerr;
            cwd     = options.cwd;
            env     = options.env;
            timeout = options.timeout;
        }
        RunOptions(const RunOptions&)=default;
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

        operator PopenOptions() const {
            PopenOptions options;
            options.check   = check;
            options.cin     = cin;
            options.cout    = cout;
            options.cerr    = cerr;
            options.cwd     = cwd;
            options.env     = env;
            options.timeout = timeout;
            return options;
        }
    };
    class ProcessBuilder;
    /** Active running process.

        Similar design of subprocess.Popen. In c++ I didn't like
    */
    struct Popen {
    public:
        Popen(){}
        Popen(CommandLine command, const PopenOptions& options);
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
        /** Waits for process to finish.

            If stdout or stderr is not read from, the child process may be
            blocked when it tries to write to the respective streams. You
            must ensure you continue to read from stdout/stderr if it's piped
            or simply close those handles. Do this to prevent deadlock.

            @param timeout  timeout in seconds. Raises TimeoutExpired on
                            timeout. NOT IMPLEMENTED, WILL WAIT FOREVER.
            @return returncode
        */
        int wait(double timeout=0);
        /** Send the signal to the process.

            On windows SIGTERM is an alias for terminate()
        */
        bool send_signal(int signal);
        /** Sends SIGTERM, on windows calls TerminateProcess() */
        bool terminate();
        /** sends SIGKILL on linux, alias for terminate() on windows.
        */
        bool kill();

        /** Destructs the object and initializes to basic state */
        void close();
        friend ProcessBuilder;
    private:
#ifdef _WIN32
        PROCESS_INFORMATION process_info;
#endif
    };



    /** This class does the bulk of the work for starting a process. It is the
        most customizable and hence the most complex.
    */
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
        std::string windows_args(const CommandLine& command);

        Popen run() {
            return run_command(this->command);
        }
        Popen run_command(const CommandLine& command);
    };

    CompletedProcess run(CommandLine command, RunOptions options={});
    CompletedProcess run_popen(CommandLine command, PopenOptions options={});

    struct RunBuilder {
        RunOptions options;
        CommandLine command;

        RunBuilder(CommandLine cmd) : command(cmd){}
        RunBuilder& check(bool ch) {options.check = ch; return *this;}
        RunBuilder& cin(PipeOption cin) {options.cin = cin; return *this;}
        RunBuilder& cout(PipeOption cout) {options.cout = cout; return *this;}
        RunBuilder& cerr(PipeOption cerr) {options.cerr = cerr; return *this;}
        RunBuilder& cwd(std::string cwd) {options.cwd = cwd; return *this;}
        RunBuilder& env(EnvMap& env) {options.env = env; return *this;}
        RunBuilder& timeout(double timeout) {options.timeout = timeout; return *this;}

        operator RunOptions() const {return options;}
        operator PopenOptions() const {return options;}

        CompletedProcess run() {return subprocess::run(command, options);}
        Popen popen() { return Popen(command, options); }
    };

    /** Like RunBuilder but defaults to capturing stdout */
    struct PopenBuilder {
        PopenOptions options;
        CommandLine command;

        PopenBuilder(){}
        PopenBuilder(CommandLine cmd) : command(cmd){}

        PopenBuilder& check(bool ch) {options.check = ch; return *this;}
        PopenBuilder& cin(PipeOption cin) {options.cin = cin; return *this;}
        PopenBuilder& cout(PipeOption cout) {options.cout = cout; return *this;}
        PopenBuilder& cerr(PipeOption cerr) {options.cerr = cerr; return *this;}
        PopenBuilder& cwd(std::string cwd) {options.cwd = cwd; return *this;}
        PopenBuilder& env(EnvMap& env) {options.env = env; return *this;}
        PopenBuilder& timeout(double timeout) {options.timeout = timeout; return *this;}

        operator PopenOptions() const {return options;}
        operator RunOptions() const {return options;}

        CompletedProcess run() {return subprocess::run(command, options);}
        Popen popen() { return Popen(command, options); }
    };

}