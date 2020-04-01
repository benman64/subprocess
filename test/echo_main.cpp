#include <cstdio>
#include <cstring>
// no echo on windows, so we make this to help test the library
int main(int argc, char** argv) {
    bool print_space = false;
    for (int i = 1; i < argc; ++i) {
        if (print_space)
            fwrite(" ", 1, 1, stdout);
        fwrite(argv[i], 1, strlen(argv[i]), stdout);
        print_space = true;
    }
    fwrite("\n", 1, 1, stdout);
    return 0;
}