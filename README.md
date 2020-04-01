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

```cpp
#include <subprocess.hpp>

// quick echo it, doesn't capture
subprocess::run({"echo", "hello", "world"});

// simplest capture output.
CompletedProcess process = subprocess::capture({"echo", "hello", "world"});

// capture stderr too.
CompletedProcess process = subprocess::capture({"echo", "hello", "world"},
    PopenBuilder().cerr(PipeOption::pipe)
    .check(true) // will throw CalledProcessError if returncode != 0.
);


// capture output. You can do this syntax if you have C++20
CompletedProcess process = subprocess::run({"echo", "hello", "world"}, {
    .cout = PipeOption::pipe,
    // make true to throw exception
    .check = false
});

std::cout << "captured: " << process.cout << '\n';
```

# Design

stdin, stdout, stderr are macros, so it's not possible to use those variable
names. Instead c++ variable names is used cin, cout, cerr where the std* would
have been respecrtivly.

PopenBuilder & RunBuilder can be used interchangebly. However PopenBuilder
defaults to automaticly setting cout to be captured.

# current progress

Hello world test is passing on macos, linux, and cross compiling on linux for
windows using mingw.

must to be implemented

- tests
- timeout support
- documentation
- bugs, there is lots of them to be discovered.
- main structure is set in place and help is welcome.

# not good

- due to types c++ is more wordy
