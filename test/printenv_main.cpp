#include <iostream>
#include <subprocess.hpp>

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
#endif

// no echo on windows, so we make this to help test the library
int main(int argc, char** argv) {
    #ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
    #endif
    if (argc != 2) {
        std::cout << "printenv <var-name>\n    Will print out contents of that variable\n";
        std::cout << "    Returns failure code if variable was not found.\n";
        return 1;
    }
    std::string result = subprocess::cenv[argv[1]];
    if (result.empty())
        return 1;
    std::cout << result << "\n";
    return 0;
}