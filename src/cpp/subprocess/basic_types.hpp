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
    // ssize_t is not a standard type and not supported in MSVC
    typedef intptr_t ssize_t;


    #ifdef _WIN32
    /** True if on windows platform. This constant is useful so you can use
        regular if statements instead of ifdefs and have both branches compile
        therebye reducing chance of compiler error on a different platform.
    */
    constexpr bool kIsWin32 = true;
    #else
    constexpr bool kIsWin32 = false;
    #endif
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
    
    /** The path seperator for PATH environment variable. */
    constexpr char kPathDelimiter = ':';
    // to please windows we can't have this be a constexpr and be standard c++
    /** The value representing an invalid pipe */
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

    /** The value representing an invalid exit code possible for a process. */
    constexpr int kBadReturnCode = -1000;

    typedef std::vector<std::string> CommandLine;
    typedef std::map<std::string, std::string> EnvMap;

    /** Redirect destination */
    enum class PipeOption : int {
        inherit, ///< Inherits current process handle
        cout,       ///< Redirects to stdout
        cerr,       ///< redirects to stderr
        /** Redirects to provided pipe. You can open /dev/null. Pipe handle
            that you specify will be made inheritable and closed automatically.
        */
        specific,
        pipe,       ///< Redirects to a new handle created for you.
        close       ///< Troll the child by providing a closed pipe.
    };

    struct SubprocessError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct OSError : SubprocessError {
        using SubprocessError::SubprocessError;
    };

    struct CommandNotFoundError : SubprocessError {
        using SubprocessError::SubprocessError;
    };

    // when the API for spawning a process fails. I don't know if this ever
    // happens in practice.
    struct SpawnError : OSError {
        using OSError::OSError;
    };

    struct TimeoutExpired : SubprocessError {
        using SubprocessError::SubprocessError;
        /** The command that was running */
        CommandLine command;
        /** The specified timeout */
        double      timeout;

        /** Captured stdout */
        std::string cout;
        /** captured stderr */
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

    /** Details about a completed process. */
    struct CompletedProcess {
        /** The args used for the process. This includes the first first arg
            which is the command/executable itself.
        */
        CommandLine     args;
        /** negative number -N means it was terminated by signal N. */
        int             returncode = -1;
        /** Captured stdout */
        std::string     cout;
        /** Captured stderr */
        std::string     cerr;
        explicit operator bool() const {
            return returncode == 0;
        }
    };

    namespace details {
        void throw_os_error(const char* function, int errno_code);
    }
}