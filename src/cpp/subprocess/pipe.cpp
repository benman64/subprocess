#include "pipe.hpp"

#include <thread>

#ifndef _WIN32
#include <fcntl.h>
#endif

namespace subprocess {
    PipePair& PipePair::operator=(PipePair&& other) {
        close();
        const_cast<PipeHandle&>(input)   = other.input;
        const_cast<PipeHandle&>(output)  = other.output;
        other.disown();
        return *this;
    }
    void PipePair::close() {
        if (input != kBadPipeValue)
            pipe_close(input);
        if (output != kBadPipeValue)
            pipe_close(output);
        disown();
    }
    void PipePair::close_input() {
        if (input != kBadPipeValue) {
            pipe_close(input);
            const_cast<PipeHandle&>(input) = kBadPipeValue;
        }
    }
    void PipePair::close_output() {
        if (output != kBadPipeValue) {
            pipe_close(output);
            const_cast<PipeHandle&>(output) = kBadPipeValue;
        }
    }
#ifdef _WIN32
    bool pipe_set_inheritable(subprocess::PipeHandle handle, bool inheritable) {
        return !!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, inheritable? HANDLE_FLAG_INHERIT : 0);
    }
    bool pipe_close(PipeHandle handle) {
        return !!CloseHandle(handle);
    }
    PipePair pipe_create(bool inheritable) {
        SECURITY_ATTRIBUTES security = {0};
        security.nLength = sizeof(security);
        security.bInheritHandle = inheritable;
        PipeHandle input, output;
        bool result = CreatePipe(&input, &output, &security, 0);
        if (!result) {
            input = output = kBadPipeValue;
            throw std::runtime_error("could not create pipe");
        }
        return {input, output};
    }
    ssize_t pipe_read(PipeHandle handle, void* buffer, std::size_t size) {
        DWORD bread = 0;
        bool result = ReadFile(handle, buffer, size, &bread, nullptr);
        if (result)
            return bread;
        return -1;
    }

    ssize_t pipe_write(PipeHandle handle, const void* buffer, size_t size) {
        DWORD written = 0;
        bool result = WriteFile(handle, buffer, size, &written, nullptr);
        if (result)
            return written;
        return -1;
    }

#else
    bool pipe_set_inheritable(PipeHandle handle, bool inherits) {
        if (handle == kBadPipeValue)
            return false;
        fcntl(handle, F_SETFD, inherits? 0 : FD_CLOEXEC);
        return true;
    }
    bool pipe_close(PipeHandle handle) {
        if (handle == kBadPipeValue)
            return false;
        return ::close(handle) == 0;
    }

    PipePair pipe_create(bool inheritable) {
        int fd[2];
        bool success =!::pipe(fd);
        if (!success) {
            throw std::runtime_error("could not create pipe");
            return {};
        }
        if (!inheritable) {
            pipe_set_inheritable(fd[0], false);
            pipe_set_inheritable(fd[1], false);
        }
        return {fd[0], fd[1]};
    }

    ssize_t pipe_read(PipeHandle handle, void* buffer, size_t size) {
        return ::read(handle, buffer, size);
    }

    ssize_t pipe_write(PipeHandle handle, const void* buffer, size_t size) {
        return ::write(handle, buffer, size);
    }
#endif

    std::string pipe_read_all(PipeHandle handle) {
        if (handle == kBadPipeValue)
            return {};
        constexpr int buf_size = 2048;
        uint8_t buf[buf_size];
        std::string result;
        while(true) {
            ssize_t transfered = pipe_read(handle, buf, buf_size);
            if(transfered > 0) {
                result.insert(result.end(), &buf[0], &buf[transfered]);
            } else {
                break;
            }
        }
        return result;
    }

    void pipe_ignore_and_close(PipeHandle handle) {
        if (handle == kBadPipeValue)
            return;
        std::thread thread([handle]() {
            std::vector<uint8_t> buffer(1024);
            while(pipe_read(handle, &buffer[0], buffer.size()) >= 0){
            }
            pipe_close(handle);
        });
        thread.detach();
    }


}