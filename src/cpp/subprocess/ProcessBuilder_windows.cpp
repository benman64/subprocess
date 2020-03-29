#ifdef _WIN32
// because of how different it is, windows gets it's own file
#include "ProcessBuilder.hpp"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include "shell.hpp"
#include "exceptions.hpp"

static STARTUPINFO g_startupInfo;
static bool g_startupInfoInit = false;


static void init_startup_info() {
    if(g_startupInfoInit)
        return;
    GetStartupInfo(&g_startupInfo);
}



namespace tea {

    Popen ProcessBuilder::run_command(const CommandLine& command) {
        std::string program = find_program(command[0]);
        if(program.empty()) {
            throw CommandNotFoundError("command not found " + command[0]);
        }
        init_startup_info();

        Popen process;
        PipePair cin_pair;
        PipePair cout_pair;
        PipePair cerr_pair;
        PipePair closed_pair;

        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited.

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;


        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        BOOL bSuccess = FALSE;

        // Set up members of the PROCESS_INFORMATION structure.

        ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

        // Set up members of the STARTUPINFO structure.
        // This structure specifies the STDIN and STDOUT handles for redirection.

        ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError   = g_startupInfo.hStdError;
        siStartInfo.hStdOutput  = g_startupInfo.hStdOutput;
        siStartInfo.hStdInput   = g_startupInfo.hStdInput;
        siStartInfo.dwFlags     |= STARTF_USESTDHANDLES;

        if (cin_option == PipeOption::close) {
            cin_pair = pipe_create();
            siStartInfo.hStdInput = cin_pair.input;
        } else if (cin_option == PipeOption::specific) {
            siStartInfo.hStdInput = cin_pipe;
        } else if (cin_option == PipeOption::pipe) {
            cin_pair = pipe_create();
            siStartInfo.hStdInput = cin_pair.input;
            process.cin = cout_pair.output;
        }


        if (cout_option == PipeOption::close) {
            cout_pair = pipe_create();
            siStartInfo.hStdOutput = cout_pair.output;
        } else if (cout_option == PipeOption::pipe) {
            cout_pair = pipe_create();
            siStartInfo.hStdOutput = cout_pair.output;
            process.cout = cout_pair.input;
        } else if (cout_option == PipeOption::cerr) {
            // Do this when stderr is setup bellow
        } else if (cout_option == PipeOption::specific) {
            siStartInfo.hStdOutput = cout_pipe;
        }

        if (cerr_option == PipeOption::close) {
            cerr_pair = pipe_create();
            siStartInfo.hStdError = cout_pair.output;
        } else if (cerr_option == PipeOption::pipe) {
            cerr_pair = pipe_create();
            siStartInfo.hStdError = cerr_pair.output;
            process.cerr = cerr_pair.input;
        } else if (cerr_option == PipeOption::cout) {
            siStartInfo.hStdError = siStartInfo.hStdOutput;
        } else if (cerr_option == PipeOption::specific) {
            siStartInfo.hStdError = cerr_pipe;
        }

        // I don't know why someone would want to do this. But for completeness
        if (cout_option == PipeOption::cerr) {
            siStartInfo.hStdOutput = siStartInfo.hStdError;
        }

        std::string args = windows_args(command);
        // Create the child process.
        bSuccess = CreateProcess(program.c_str(),
            (char*)args.c_str(),     // command line
            NULL,          // process security attributes
            NULL,          // primary thread security attributes
            TRUE,          // handles are inherited
            0,             // creation flags
            NULL,          // use parent's environment
            NULL,          // use parent's current directory
            &siStartInfo,  // STARTUPINFO pointer
            &piProcInfo);  // receives PROCESS_INFORMATION

        if (cin_pair)
            pipe_close(cin_pair.input);
        if (cout_pair)
            pipe_close(cout_pair.output);
        if (cerr_pair)
            pipe_close(cerr_Pair.output);

        if (cin_option == PipeOption::close)
            pipe_close(cin_pair.output);
        if (cout_option == PipeOption::close)
            pipe_close(cout_pair.input);
        if (cerr_option == PipeOption::close)
            pipe_option(cerr_pair.input);

        if ( !bSuccess )
            return {}; // TODO throw SpawnError

        return process;
    }


}

void ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box,
// and exit from the application.
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}

#endif