#ifdef _WIN32
// because of how different it is, windows gets it's own file
#include "ProcessBuilder.hpp"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#include "shell_utils.hpp"
#include "environ.hpp"

static STARTUPINFO g_startupInfo;
static bool g_startupInfoInit = false;


static void init_startup_info() {
    if(g_startupInfoInit)
        return;
    GetStartupInfo(&g_startupInfo);
}

bool disable_inherit(subprocess::PipeHandle handle) {
    return !!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0);
}

namespace subprocess {

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

        SECURITY_ATTRIBUTES saAttr = {0};

        // Set the bInheritHandle flag so pipe handles are inherited.

        saAttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle       = TRUE;
        saAttr.lpSecurityDescriptor = NULL;


        PROCESS_INFORMATION piProcInfo  = {0};
        STARTUPINFO siStartInfo         = {0};
        BOOL bSuccess = FALSE;

        siStartInfo.cb          = sizeof(STARTUPINFO);
        siStartInfo.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.hStdOutput  = GetStdHandle(STD_OUTPUT_HANDLE);
        siStartInfo.hStdError   = GetStdHandle(STD_ERROR_HANDLE);
        siStartInfo.dwFlags    |= STARTF_USESTDHANDLES;

        if (cin_option == PipeOption::close) {
            cin_pair = pipe_create();
            siStartInfo.hStdInput = cin_pair.input;
            disable_inherit(cin_pair.output);
        } else if (cin_option == PipeOption::specific) {
            pipe_set_inheritable(cin_pipe, true);
            siStartInfo.hStdInput = cin_pipe;
        } else if (cin_option == PipeOption::pipe) {
            cin_pair = pipe_create();
            siStartInfo.hStdInput = cin_pair.input;
            process.cin = cin_pair.output;
            disable_inherit(cin_pair.output);
        }


        if (cout_option == PipeOption::close) {
            cout_pair = pipe_create();
            siStartInfo.hStdOutput = cout_pair.output;
            disable_inherit(cout_pair.input);
        } else if (cout_option == PipeOption::pipe) {
            cout_pair = pipe_create();
            siStartInfo.hStdOutput = cout_pair.output;
            process.cout = cout_pair.input;
            disable_inherit(cout_pair.input);
        } else if (cout_option == PipeOption::cerr) {
            // Do this when stderr is setup bellow
        } else if (cout_option == PipeOption::specific) {
            pipe_set_inheritable(cout_pipe, true);
            siStartInfo.hStdOutput = cout_pipe;
        }

        if (cerr_option == PipeOption::close) {
            cerr_pair = pipe_create();
            siStartInfo.hStdError = cerr_pair.output;
            disable_inherit(cerr_pair.input);
        } else if (cerr_option == PipeOption::pipe) {
            cerr_pair = pipe_create();
            siStartInfo.hStdError = cerr_pair.output;
            process.cerr = cerr_pair.input;
            disable_inherit(cerr_pair.input);
        } else if (cerr_option == PipeOption::cout) {
            siStartInfo.hStdError = siStartInfo.hStdOutput;
        } else if (cerr_option == PipeOption::specific) {
            pipe_set_inheritable(cerr_pipe, true);
            siStartInfo.hStdError = cerr_pipe;
        }

        // I don't know why someone would want to do this. But for completeness
        if (cout_option == PipeOption::cerr) {
            siStartInfo.hStdOutput = siStartInfo.hStdError;
        }
        const char* cwd = this->cwd.empty()? nullptr : this->cwd.c_str();
        std::string args = windows_args(command);

        void* env = nullptr;
        std::u16string envblock;
        if (!this->env.empty()) {
            /*  if you use ansi there is a 37K size limit. So we use unicode
                which is almost utf16.

                TODO: fix by using unicode 16bit chars.

                This won't work as expected if somewhere there is a multibyte
                utf-16 char (4-bytes total).
            */
            envblock = create_env_block(this->env);
            env = (void*)envblock.data();
        }
        DWORD process_flags = CREATE_UNICODE_ENVIRONMENT;
        if (this->new_process_group) {
            process_flags |= CREATE_NEW_PROCESS_GROUP;
        }
        // Create the child process.
        bSuccess = CreateProcess(program.c_str(),
            (char*)args.c_str(),            // command line
            NULL,                           // process security attributes
            NULL,                           // primary thread security attributes
            TRUE,                           // handles are inherited
            process_flags,                  // creation flags
            env,                            // environment
            cwd,                            // use parent's current directory
            &siStartInfo,                   // STARTUPINFO pointer
            &piProcInfo);                   // receives PROCESS_INFORMATION
        process.process_info = piProcInfo;
        process.pid = piProcInfo.dwProcessId;
        if (cin_pair)
            cin_pair.close_input();
        if (cout_pair)
            cout_pair.close_output();
        if (cerr_pair)
            cerr_pair.close_output();

        if (cin_option == PipeOption::close)
            cin_pair.close();
        if (cout_option == PipeOption::close)
            cout_pair.close();
        if (cerr_option == PipeOption::close)
            cerr_pair.close();

        cin_pair.disown();
        cout_pair.disown();
        cerr_pair.disown();

        process.args = command;
        // TODO: get error and add it to throw
        if (!bSuccess )
            throw SpawnError("CreateProcess failed");
        return process;
    }


}

#endif