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
#include <csignal>


// Fucking stdout, stderr, stdin are macros. So instead of stdout,...
// we will use cin, cout, cerr as variable names


namespace subprocess {
    /*  windows doesnt'h have all of these. The numeric values I hope are
        standardized. Posix specifies the number in the standard so most
        systems should be fine.
    */

    /** Signals to send. they start with P because SIGX are macros, P
        stands for Posix as these values are as defined by Posix.
    */
    enum SigNum {
        PSIGHUP     = 1,
        PSIGINT     = SIGINT,
        PSIGQUIT    = 3,
        PSIGILL     = SIGILL,
        PSIGTRAP    = 5,
        PSIGABRT    = SIGABRT,
        PSIGIOT     = 6,
        PSIGBUS     = 7,
        PSIGFPE     = SIGFPE,
        PSIGKILL    = 9,
        PSIGUSR1    = 10,
        PSIGSEGV    = SIGSEGV,
        PSIGUSR2    = 12,
        PSIGPIPE    = 13,
        PSIGALRM    = 14,
        PSIGTERM    = SIGTERM,
        PSIGSTKFLT  = 16,
        PSIGCHLD    = 17,
        PSIGCONT    = 18,
        PSIGSTOP    = 19,
        PSIGTSTP    = 20,
        PSIGTTIN    = 21,
        PSIGTTOU    = 22,
        PSIGURG     = 23,
        PSIGXCPU    = 24,
        PSIGXFSZ    = 25,
        PSIGVTALRM  = 26,
        PSIGPROF    = 27,
        PSIGWINCH   = 28,
        PSIGIO      = 29
    };
#ifndef _WIN32
    typedef int PipeHandle;
    typedef ::pid_t pid_t;

    constexpr char kPathDelimiter = ':';
    // to please windows we can't have this be a constexpr and be standard c++
    const PipeHandle kBadPipeValue = (PipeHandle)-1;
#else
    typedef HANDLE PipeHandle;
    typedef DWORD pid_t;

    constexpr char kPathDelimiter = ';';
    const PipeHandle kBadPipeValue = INVALID_HANDLE_VALUE;
#endif
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