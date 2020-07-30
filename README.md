# subprocess
cross platform subprocess library for c++ similar to design of python
subprocess. See [subprocess documentation](https://benman64.github.io/subprocess/index.html)
for further documentation.


# supports

- very python like style of subprocess. With very nice syntax for c++20.
- Connect output of process A to input of process B. However not pretty API for
  this.

## Shakey elements

- The os error level exceptions is still changing. I'm thinking of having an
  OSError subclass to abstract the OS differences.
# requirements

- c++17
- linked with support for threading, filesystem

# Integration

##  Adhoc

1. copy files in src/cpp to your project.
2. add the top folder as include.
3. make sure cpp files are compiled.
4. add `#include <subprocess.hpp>` to start using in source files.

## [Teaport](https://bitbucket.org/benman/teaport)

add this to your dependencies:

```
"subprocess": "0.1.+"
```

## Todo add to cocoapods and perhaps others.

# Examples

```cpp
#include <subprocess.hpp>
#include <thread>
#include <cstring>

void simple() {
    using subprocess::CompletedProcess;
    using subprocess::RunBuilder;
    using subprocess::PipeOption;
    // quick echo it, doesn't capture
    subprocess::run({"echo", "hello", "world"});

    // simplest capture output.
    CompletedProcess process = subprocess::run({"echo", "hello", "world"},
        RunBuilder().cout(PipeOption::pipe));

    // simplest sending data example
    process = subprocess::run({"cat"},
        RunBuilder().cin("hello world\n"));

    // simplest send & capture
    process = subprocess::run({"cat"},
        RunBuilder().cin("hello world").cout(PipeOption::pipe));
    std::cout << "captured: " << process.cout << '\n';

    // capture stderr too.
    process = subprocess::run({"echo", "hello", "world"},
        RunBuilder().cerr(PipeOption::pipe)
        .cout(PipeOption::pipe)
        .check(true) // will throw CalledProcessError if returncode != 0.
    );

    // there is no cerr so it will be empty
    std::cout << "cerr was: " << process.cerr << "\n";

#if __cplusplus >= 202002L
    // capture output. You can do this syntax if you have C++20
    process = subprocess::run({"echo", "hello", "world"}, {
        .cout = PipeOption::pipe,
        // make true to throw exception
        .check = false
    });

    std::cout << "captured: " << process.cout << '\n';
#endif
}


void popen_examples() {
    using subprocess::CompletedProcess;
    using subprocess::RunBuilder;
    using subprocess::Popen;
    using subprocess::PipeOption;

    // simplest example
    // capture is enabled by default
    Popen popen = subprocess::RunBuilder({"echo", "hello", "world"})
        .cout(PipeOption::pipe).popen();
    char buf[1024] = {0}; // initializes everything to 0
    subprocess::pipe_read(popen.cout, buf, 1024);
    std::cout << buf;
    // the destructor will call wait on your behalf.
    popen.close();


    // communicate with data
    popen = subprocess::RunBuilder({"cat"}).cin(PipeOption::pipe)
        .cout(PipeOption::pipe).popen();
    /*  if we write more data than the buffer, we would dead lock if the subprocess
        is deadlocked trying to write. So we spin a new thread for writing. When
        you provide buffers for cin, internally the library spins it's own thread.
    */
    std::thread write_thread([&]() {
        subprocess::pipe_write(popen.cin, "hello world\n", std::strlen("hello world\n"));
        // no more data to send. If we don't close we may run into a deadlock as
        // we are looking to read for more.
        popen.close_cin();
    });

    for (auto& c : buf)
        c = 0;

    subprocess::pipe_read(popen.cout, buf, 1024);
    std::cout << buf;
    popen.close();
    if (write_thread.joinable())
        write_thread.join();
}
int main(int argc, char** argv) {
    std::cout << "running basic examples\n";
    simple();
    std::cout << "running popen_examples\n";
    popen_examples();
    return 0;
}
```

# Design

stdin, stdout, stderr are macros, so it's not possible to use those variable
names. Instead c++ variable names is used cin, cout, cerr where the std* would
have been respectively.

PopenBuilder was a bad idea. Removed, now only RunBuilder remains which is enough.
Have both is too confusing.

# current progress

all tests pass on linux & mac. Most pass under mingw. MSVC all tests pass but
I don't believe it because mingw has some fail cases. Must be a bug in some of
the tests.

# not good

- due to types c++ is more wordy
- should termination deviate from what python does? Python sends the equivalent
  SIGKILL on windows. I don't understand why, and it makes sense to send CTRL+Break.
  Being different may be a surprise to python developers. But not being different
  will be a surprise to people not aware of what python subprocess does.

# Changelog

# 0.3.0 coming soon

- fixed MSVC issues & compiles
- documentation should be complete. Please report any missing

# 0.2.0

- omg setting check=true is fixed. What a typo

