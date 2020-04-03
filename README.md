# subprocess
cross platform subprocess library for c++ similar to design of python
subprocess.

# requirements

- c++17

# Integration

##  Adhoc

1. copy files in src/cpp to your project.
2. add the top folder as include.
3. make sure cpp files are compiled.
4. add `#include <subprocess.hpp>` to start using in source files.

## Todo add to cocoapods and perhaps others.

# Examples

## Simple examples

```cpp
#include <subprocess.hpp>

// quick echo it, doesn't capture
subprocess::run({"echo", "hello", "world"});

// simplest capture output.
CompletedProcess process = subprocess::run({"echo", "hello", "world"},
    RunBuilder.cout(PipeOption::pipe));

// simplest sending data example
CompletedProcess process = subprocess::run({"cat"},
    RunBuilder().cin("hello world"));

// simplest send & capture
CompletedProcess process = subprocess::run({"cat"},
    PopenBuilder().cin("hello world").cout(PipeOption::pipe));
std::cout << "captured: " << process.cout << '\n';

// capture stderr too.
CompletedProcess process = subprocess::run({"echo", "hello", "world"},
    PopenBuilder().cerr(PipeOption::pipe)
    .cout(PipeOption::pipe)
    .check(true) // will throw CalledProcessError if returncode != 0.
);
std::cout << "cerr was: " << process.cerr;

// capture output. You can do this syntax if you have C++20
CompletedProcess process = subprocess::run({"echo", "hello", "world"}, {
    .cout = PipeOption::pipe,
    // make true to throw exception
    .check = false
});

std::cout << "captured: " << process.cout << '\n';
```

## Popen examples

These examples show using the library while the subprocess is still running.

```cpp

// simplest example
// capture is enabled by default
Popen popen = subprocess::Popen({"echo", "hello", "world"}, RunBuilder().cout(PipeOption::pipe));
char buf[1024] = {0}; // initializes everything to 0
subprocess::pipe_read(popen.cout, buffer, 1024);
std::cout << buf;
// the destructor will call wait on your behalf.
popen.close();


// communicate with data
Popen popen = subprocess::Popen({"cat"}, RunBuilder().cin(PipeOption::pipe).cout(PipeOption::pipe));
/*  if we write more data than the buffer, we would dead lock if the subprocess
    is deadlocked trying to write. So we spin a new thread for writing. When
    you provide buffers for cin, internally the library spins it's own thread.
*/
std::thread write_thread([&]() {
    pipe_write(popen.cin, "hello world\n", strlen("hello world\n"));
    // no more data to send. If we don't close we may run into a deadlock as
    // we are looking to read for more.
    popen.cin_close();
});
char buf[1024] = {0}; // initializes everything to 0
subprocess::pipe_read(popen.cout, buffer, 1024);
std::cout << buf;
popen.close();
if (write_thread.joinable())
    write_thread.join();
```

# Design

stdin, stdout, stderr are macros, so it's not possible to use those variable
names. Instead c++ variable names is used cin, cout, cerr where the std* would
have been respectively.

PopenBuilder was a bad idea. Removed, now only RunBuilder remains which is enough.
Have both is too confusing.

# current progress

Hello world tests are passing on macos, linux, and cross compiling on linux for
windows using mingw. The library is almost feature complete.

must to be implemented

- tests
- documentation
- bugs, there is lots of them to be discovered.
- main structure is set in place and help is welcome.

# not good

- due to types c++ is more wordy
