#include <cstdio>
#include <cstring>
#include <chrono>
#include <charconv>

void sleep_seconds(double seconds) {
    std::chrono::duration<double> duration(seconds);
    std::this_thread::sleep_for(duration);
}

// no echo on windows, so we make this to help test the library
int main(int argc, char** argv) {
    bool print_space = false;
    if (argc != 2)
        return 1;
    double seconds = 0;
    std::from_chars(argv[1], argv[1] + strlen(argv[1]), seconds);
    sleep_seconds(seconds);
    return 0;
}