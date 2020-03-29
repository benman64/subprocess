#pragma once

#include "basic_types.hpp"

namespace subprocess {
    struct PipePair {
        PipeHandle input    = kBadPipeValue;
        PipeHandle output   = kBadPipeValue;

        explicit operator bool() const noexcept {
            return input != output;
        }
    };

    /** Closes a pipe handle.
        @param handle   The handle to close.
        @returns true on success
    */
    bool pipe_close(PipeHandle handle);
    PipePair pipe_create(bool inheritable = true);
    bool pipe_set_no_inherit(PipeHandle handle);

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
}