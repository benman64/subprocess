# subprocess
cross platform subprocess library for c++ similar to design of python
subprocess. See [subprocess documentation](https://benman64.github.io/subprocess/index.html)
for further documentation.


# supports

- very python like style of subprocess. With very nice syntax for c++20.
- Connect output of process A to input of process B. However not pretty API for
  this.
- Environment utilities to make it easy to get/set environment variables. as
  easy as `subprocess::cenv["MY_VAR"] = "value"`.
- subprocess::EnvGuard that will save the environment and reload it when scope
  block ends, making it easy to have a temporary environment. Obviously this is
  not thread safe as environment variable changes effects process wide.
- Get a copy of environment so you can modify a std::map as you please for use
  in a thread safe manner of environment and pass it along to subprocesses.
- cross-platform `find_program`
- find_program has special handling of "python3" on windows making it easy to
  find python3 executable. It searches the path for python and inspects it's
  version so that `find_program("python3")` is cross-platform.
- Supports connecting process stdin, stdout, stderr to C++ streams making
  redirection convenient. stdin can be connected with a std::string too.

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
"subprocess": "0.4.+"
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

# Deviations

- On windows terminating a process sends CTRL_BREAK_EVENT instead of hard
  termination. You can send a SIGKILL and it will do a hard termination as
  expected. Becareful as this may kill your process as it's sent to the process
  group. See send_signal for more details.
- cin, cout, cerr variable names are used instead of stdin, stdout, stderr as
  std* are macros and cannot be used as names in C++.

# current progress

All tests pass on linux & mac. Most pass under mingw & MSVC.


# Changelog

# 0.5.0 TBA

- fixed #16 is_drive had a typo and so lowercase drives weren't properly
  interpretted.
- fixed #2 subprocess.run() respects timeout passed in.
- breaking: RunOptions which is used in subprocess::run order is changed to be
  identical to python subprocess::run. This effects users using c++20 designated
  initializers. Prior versions of compilers didn't seem to care about order.
- fixed #5 cin double closed.

# 0.4.0

- `CTRL_BREAK_EVENT` is sent for SIGTERM & terminate() functions on windows.
- fixed invalid handles when launching a python script that then launches new
  processes.
- new `kIsWin32` constant to help avoid ifdef use.
- Documentation wording to be more confident as the library is looking pretty
  good, and I haven't felt like changing much of the API.

# 0.3.0

- fixed MSVC issues & compiles
- documentation should be complete. Please report any missing

# 0.2.0

- omg setting check=true is fixed. What a typo
