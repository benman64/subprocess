#pragma once

#include "basic_types.hpp"

namespace subprocess {
    /*  You might be wondering why the C-like API. I played around with a more
        C++ like API but it gets hairy quite fast trying to support all the
        possible use-cases. My opinion is to simply roll a RAII class for
        when you need it that is specific to your needs. 1, or a set of RAII
        classes is too complex.
    */
    struct PipePair {
        PipePair(){};
        PipePair(PipeHandle input, PipeHandle output) : input(input), output(output) {}
        ~PipePair(){ close(); }
        // No copy, move only
        PipePair            (const PipePair&)=delete;
        PipePair& operator= (const PipePair&)=delete;
        PipePair            (PipePair&& other) { *this = std::move(other); }
        PipePair& operator= (PipePair&& other);
        /*  we make it const as outside of code shouldn't modify these.
            this might not be a good design as users may assume it's truly const.
            disown, close*, and move semantics overwrite these values.
        */
        const PipeHandle input    = kBadPipeValue;
        const PipeHandle output   = kBadPipeValue;

        /** Stop owning the pipes */
        void disown() {
            const_cast<PipeHandle&>(input) = const_cast<PipeHandle&>(output) = kBadPipeValue;
        }
        void close();
        void close_input();
        void close_output();
        explicit operator bool() const noexcept {
            return input != output;
        }
    };

    /** Closes a pipe handle.
        @param handle   The handle to close.
        @returns true on success
    */
    bool pipe_close(PipeHandle handle);
    /** Creates a pair of pipes for input/output

        @param inheritable  if true subprocesses will inherit the pipe.

        @throw OSError if system call fails.

        @return pipe pair. If failure returned pipes will have values of kBadPipeValue
    */
    PipePair pipe_create(bool inheritable = true);
    /** Set the pipe to be inheritable or not for subprocess.

        @throw OSError if system call fails.

        @param inheritable if true handle will be inherited in subprocess.
    */
    void pipe_set_inheritable(PipeHandle handle, bool inheritable);

    /**
        @returns    -1 on error. if 0 it could be the end, or perhaps wait for
                    more data.
    */
    ssize_t pipe_read(PipeHandle, void* buffer, size_t size);
    /**
        @returns    -1 on error. if 0 it could be full, or perhaps wait for
                    more data.
    */
    ssize_t pipe_write(PipeHandle, const void* buffer, size_t size);
    /** Spawns a thread to read from the pipe. When no more data available
        pipe will be closed.
    */
    void pipe_ignore_and_close(PipeHandle handle);
    /** Read contents of handle until no more data is available.

        If the pipe is non-blocking this will end prematurely.

        @return all data read from pipe as a string object. This works fine
                with binary data.
    */
    std::string pipe_read_all(PipeHandle handle);
}