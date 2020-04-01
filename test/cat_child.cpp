#include <iostream>
#include <vector>
#include <string>
#ifdef _WIN32
#include <io.h>
//const auto& read = _read;
#else
#include <unistd.h>
#endif
template<typename T, typename V>
bool contains(const std::vector<T>& container, const V& value) {
    for (auto& test : container) {
        if (test == value)
            return true;
    }
    return false;
}

enum ExitCode {
    success = 0,
    cout_fail,
    cerr_fail,
    cout_cerr_fail,
    exception_thrown
};
int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    bool output_err = contains(args, "--output-stderr");
    std::vector<char> buffer(2048);
    try {
        while (true) {
            auto transfered = read(0, &buffer[0], buffer.size());
            if (transfered == 0)
                break;
            if (output_err) {
                std::cerr.write(&buffer[0], transfered);
                std::cerr.flush();
            } else {
                std::cout.write(&buffer[0], transfered);
                std::cout.flush();
            }
        }
    } catch (...) {
        return ExitCode::exception_thrown;
    }

    if (std::cout.fail() && std::cout.fail())
        return ExitCode::cout_cerr_fail;
    if (std::cout.fail())
        return ExitCode::cout_fail;
    if (std::cerr.fail())
        return ExitCode::cerr_fail;

    return ExitCode::success;
}