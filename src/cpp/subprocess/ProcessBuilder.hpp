#pragma once

#include <vector>
#include <string>

#include "pipe.hpp"
#include "PipeVar.hpp"

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

    struct RunOptions {
        bool        check   = false;
        PipeVar     cin     = PipeOption::inherit;
        PipeVar     cout    = PipeOption::inherit;
        PipeVar     cerr    = PipeOption::inherit;

        std::string cwd;
        /** If empty inherits from current process */
        EnvMap      env;

        /** Timeout in seconds. Raise TimeoutExpired.

            Only available if you use subprocess_run
        */
        double timeout  = -1;
    };
    class ProcessBuilder;
    /** Active running process.

        Similar design of subprocess.Popen. In c++ I didn't like
    */
    struct Popen {
    public:
        Popen(){}
        Popen(CommandLine command, const RunOptions& options);
        Popen(CommandLine command, RunOptions&& options);
        Popen(const Popen&)=delete;
        Popen& operator=(const Popen&)=delete;

        Popen(Popen&&);
        Popen& operator=(Popen&&);

        /** Waits for process, Closes pipes and destroys any handles.*/
        ~Popen();

        /** Write to this stream to send data to the process. This class holds
            the ownership and will call pipe_close().
        */
        PipeHandle  cin       = kBadPipeValue;
        /** Read from this stream to get output of process. This class holds
            the ownership and will call pipe_close().
        */
        PipeHandle  cout      = kBadPipeValue;
        /** Read from this stream to get cerr output of process. This class holds
            the ownership and will call pipe_close().
        */
        PipeHandle  cerr      = kBadPipeValue;


        pid_t       pid         = 0;
        /** The exit value of the process. Valid once process is completed */
        int         returncode  = kBadReturnCode;
        CommandLine args;

        /** calls pipe_ignore_and_close on cout */
        void ignore_cout() { pipe_ignore_and_close(cout); cout = kBadPipeValue; }
        /** calls pipe_ignore_and_close on cerr */
        void ignore_cerr() { pipe_ignore_and_close(cerr); cerr = kBadPipeValue; }
        /** calls pipe_ignore_and_close on cout, cerr if open */
        void ignore_output() { ignore_cout(); ignore_cerr(); }
        /** @return true if terminated. */
        bool poll();
        /** Waits for process to finish.

            If stdout or stderr is not read from, the child process may be
            blocked when it tries to write to the respective streams. You
            must ensure you continue to read from stdout/stderr. Call
            ignore_output() to spawn threads to ignore the output preventing a
            deadlock. You can also troll the child by closing your end.

            @param timeout  timeout in seconds. Raises TimeoutExpired on
                            timeout. NOT IMPLEMENTED, WILL WAIT FOREVER.
            @return returncode
        */
        int wait(double timeout=-1);
        /** Send the signal to the process.

            On windows SIGTERM is an alias for terminate()
        */
        bool send_signal(int signal);
        /** Sends SIGTERM, on windows calls TerminateProcess() */
        bool terminate();
        /** sends SIGKILL on linux, alias for terminate() on windows. */
        bool kill();

        /** Destructs the object and initializes to basic state */
        void close();
        /** Closes the cin pipe */
        void close_cin() {
            if (cin != kBadPipeValue) {
                pipe_close(cin);
                cin = kBadPipeValue;
            }
        }
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
    CompletedProcess capture(CommandLine command, RunOptions options={});

    struct RunBuilder {
        RunOptions options;
        CommandLine command;

        RunBuilder(){}
        RunBuilder(CommandLine cmd) : command(cmd){}
        RunBuilder& check(bool ch) {options.check = ch; return *this;}
        RunBuilder& cin(const PipeVar& cin) {options.cin = cin; return *this;}
        RunBuilder& cout(const PipeVar& cout) {options.cout = cout; return *this;}
        RunBuilder& cerr(const PipeVar& cerr) {options.cerr = cerr; return *this;}
        RunBuilder& cwd(std::string cwd) {options.cwd = cwd; return *this;}
        RunBuilder& env(const EnvMap& env) {options.env = env; return *this;}
        RunBuilder& timeout(double timeout) {options.timeout = timeout; return *this;}

        operator RunOptions() const {return options;}

        CompletedProcess run() {return subprocess::run(command, options);}
        Popen popen() { return Popen(command, options); }
    };

    double monotonic_seconds();
    double sleep_seconds(double seconds);

    class StopWatch {
    public:
        StopWatch() { start(); }

        void start() { mStart = monotonic_seconds(); }
        double seconds() const { return monotonic_seconds() - mStart; }
    private:
        double mStart;
    };
}