# subprocess
cross platform subprocess library for c++ similar to design of python subprocess

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

```
#include <subprocess.hpp>

// quick echo it
subprocess::run({"echo", "hello", "world"});


// capture output.
CompletedProcess process = subprocess::run({"echo", "hello", "world"},
    PopenBuilder().cout(PipeOption::pipe)
    .check(false).build()
);


// capture output. You can do this syntax if you have C++20
CompletedProcess process = subprocess::run({"echo", "hello", "world"}, {
    .cout = PipeOption::pipe,
    // make true to throw exception
    .check = false
});

std::cout << "captured: " << process.cout << '\n';
```


# current progress

must to be implemented

- tests
- timeout support
- documentation
- bugs, there is lots of them to be discovered.
- main structure is set in place and help is welcome.