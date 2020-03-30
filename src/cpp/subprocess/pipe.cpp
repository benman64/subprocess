#include "pipe.hpp"


namespace subprocess {
#ifdef _WIN32
    bool pipe_close(PipeHandle handle) {
        return !!CloseHandle(handle);
    }
    PipePair pipe_create(bool inheritable) {
        PipePair pair;
        SECURITY_ATTRIBUTES security = {0};
        security.nLength = sizeof(security);
        security.bInheritHandle = inheritable;
        bool result = CreatePipe(&pair.input, &pair.output, &security);
        return pair;
    }
    ssize_t pipe_read(PipeHandle handle, void* buffer, std::size_t size) {
        DWORD bread = 0;
        bool result = ReadFile(handle, buffer, size, &bread);
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
    bool pipe_close(PipeHandle handle) {
        if (handle == kBadPipeValue)
            return false;
        return ::close(handle) == 0;
    }

    PipePair pipe_create(bool inheritable) {
        int fd[2];
        bool success =!::pipe(fd);
        if (!success) {
            return {};
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
            size_t transfered = pipe_read(handle, buf, buf_size);
            if(transfered > 0) {
                result.insert(result.end(), &buf[0], &buf[transfered]);
            } else {
                break;
            }
        }
        return result;
    }
}