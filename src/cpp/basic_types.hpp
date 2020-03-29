#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdexcept>
#include <vector>
#include <map>

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

    struct ShellError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    struct CommandNotFoundError : ShellError {
        using ShellError::ShellError;
    };
    struct SpawnError : ShellError {
        using ShellError::ShellError;
    };

    struct TimeoutExpired : ShellError {
        using ShellError::ShellError;

        CommandLine command;
        double      timeout;

        std::string cout;
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