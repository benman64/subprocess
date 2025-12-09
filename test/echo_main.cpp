#include <cstdio>
#include <cstring>
#include <subprocess.hpp>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif

// no echo on windows, so we make this to help test the library
int main(int argc, char** argv) {
    bool print_space = false;
    #ifdef _WIN32
    std::string binary = subprocess::cenv["BINARY"];
    if (binary.size() > 0 && binary != "0" && binary != "false") {
        setmode(fileno(stdout), O_BINARY);
    }
    #endif
    std::string use_cerr_str = subprocess::cenv["USE_CERR"];
    bool use_cerr = use_cerr_str == "1";
    auto output_file = use_cerr? stderr : stdout;
    for (int i = 1; i < argc; ++i) {
        if (print_space)
            fwrite(" ", 1, 1, output_file);
        fwrite(argv[i], 1, strlen(argv[i]), output_file);
        print_space = true;
    }
    fwrite("\n", 1, 1, output_file);
    return 0;
}