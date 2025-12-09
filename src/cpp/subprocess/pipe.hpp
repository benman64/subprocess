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

        /** Disowns the input & output end */
        void disown() {
            const_cast<PipeHandle&>(input) = const_cast<PipeHandle&>(output) = kBadPipeValue;
        }
        /// Disowns the input end
        void disown_input() {
            const_cast<PipeHandle&>(input) = kBadPipeValue;
        }
        /// Disowns the output end
        void disown_output() {
            const_cast<PipeHandle&>(output) = kBadPipeValue;
        }

        void close();
        void close_input();
        void close_output();
        explicit operator bool() const noexcept {
            return input != output;
        }
    };

    /** Peak into how many bytes available in pipe to read. */
    ssize_t pipe_peak_bytes(PipeHandle pipe);

    /** Closes a pipe handle.
        @param handle   The handle to close.
        @returns true on success
    */
    bool pipe_close(PipeHandle handle);

    /** Creates a pair of pipes for input/output

        @param inheritable  if true subprocesses will inherit the pipe.

        @throw OSError if system call fails.

        @return pipe pair. If failure returned pipes will have values of kBadPipeValue

        @since 0.5.0
            inheritable default is changed from true to false. The underlaying OS API's
            make inheritable true by default and so making it not inheritble
            is extra work which is why true was default here too. But practically
            this is annoying because you only want to inherit 1 end of the pipe.
            If this is not false the child will inherit the other end and will
            never get EOF if you close your end. The subprocess API's will
            automatically enforce the correct end of the pipe to be inheritable.
            Most of the time you never have to think about this, and likely this
            breaking change will fix bug for you. If you have an explicit reason
            to make both ends to be inheritble at creation then you likely have
            a bug.
    */
    PipePair pipe_create(bool inheritable = false);

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

    /** Sets the blocking bit.

        The handle state is first queried as to only change the blocking bit.

        @param should_block
            If false then pipe_read/write will exit early if there is no blocking.

        @returns true on success
    */
    bool pipe_set_blocking(PipeHandle, bool should_block);

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

    /** Waits for the pipes to be change state.

        @param pipes
            pointer to first pipe in an array
        @param did_change
            pointer to an output of the first pipe that changed state
        @param count
            number of pipes
        @param seconds
            The timeout in seconds to wait for. -1 for indefinate
    */
    int pipe_wait_for_read(
        PipeHandle pipe,
        double seconds
    );

    /** Will read up to size and not block until buffer is filled. */
    ssize_t pipe_read_some(PipeHandle, void* buffer, size_t size);

    /** Opens a file and returns the handle.

        You must call pipe_close() on the returned handle. When using as a
        parameter to Popen or run(), pipe_close should not be called as ownership
        will be taken.

        @param filename
            The file path
        @param mode
            The mode as from fopen. Always binary mode.
            r - read only, will fail if file doesn't exist
            w - write only and will create or truncate the file
            + - allow read/write

        @returns the handle to the opened file, or kBadPipeValue on error
    */
    PipeHandle pipe_file(const char* filename, const char* mode);

    #if 0
    /*  Obviously this is possible. The problem is the API around it, I really
        want to stop calling pipe_close. To be fair in python's subprocess
        library you also have to close things either manually or use `with`.
        Maybe it's just not possible.
    */
    class UniquePipeHandle {
    public:
        UniquePipeHandle(){}
        ~UniquePipeHandle(){
            pipe_close(handle);
            handle = kBadPipeValue;
        }
        explicit UniquePipeHandle(PipeHandle handle): handle(handle){}
        UniquePipeHandle(UniquePipeHandle&& other) {
            pipe_close(handle);
            handle = other.handle;
            other.handle = kBadPipeValue;
        }
        UniquePipeHandle& operator=(UniquePipeHandle&& other) {
            pipe_close(handle);
            handle = other.handle;
            other.handle = kBadPipeValue;
            return *this;
        }
        UniquePipeHandle(const UniquePipeHandle&)=delete;
        UniquePipeHandle operator=(const UniquePipeHandle&)=delete;

        PipeHandle get() { return handle; }
    private:
        PipeHandle handle = kBadPipeValue;
    };
    #endif
}