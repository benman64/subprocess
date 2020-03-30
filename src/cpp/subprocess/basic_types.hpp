#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <map>
#include <stdexcept>
#include <string>
#include <vector>


// Fucking stdout, stderr, stdin are macros. So instead of stdout,...
// we will use cin, cout, cerr as variable names


namespace subprocess {
#ifndef _WIN32
    typedef int PipeHandle;
    typedef ::pid_t pid_t;

    constexpr char kPathDelimiter = ':';
#else
    typedef HANDLE PipeHandle;
    typedef HANDLE pid_t;

    constexpr char kPathDelimiter = ';';
#endif
    constexpr PipeHandle kBadPipeValue = (PipeHandle)-1;
    constexpr int kStdInValue   = 0;
    constexpr int kStdOutValue  = 1;
    constexpr int kStdErrValue  = 2;

    typedef std::vector<std::string> CommandLine;
    typedef std::map<std::string, std::string> EnvMap;


    enum class PipeOption : int {
        inherit,
        cout,
        cerr,
        specific,
        pipe,
        close
    };

    struct SubprocessError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    struct CommandNotFoundError : SubprocessError {
        using SubprocessError::SubprocessError;
    };

    // when the API for spawning a process fails. I don't know if this ever
    // happens in practice.
    struct SpawnError : SubprocessError {
        using SubprocessError::SubprocessError;
    };

    struct TimeoutExpired : SubprocessError {
        using SubprocessError::SubprocessError;

        CommandLine command;
        double      timeout;

        std::string cout;
        std::string cerr;
    };

    struct CalledProcessError : SubprocessError {
        using SubprocessError::SubprocessError;
        // credit for documentation is from python docs. They say it simply
        // and well.

        /** Exit status of the child process. If the process exited due to a
            signal, this will be the negative signal number.
        */
        int         returncode;
        /** Command used to spawn the child process */
        CommandLine cmd;
        /** stdout output if it was captured. */
        std::string cout;
        /** stderr output if it was captured. */
        std::string cerr;
    };

    struct CompletedProcess {
        CommandLine     args;
        /** negative number -N means it was terminated by signal N. */
        int             returncode = -1;
        std::string     cout;
        std::string     cerr;
        explicit operator bool() const {
            return returncode == 0;
        }
    };
}