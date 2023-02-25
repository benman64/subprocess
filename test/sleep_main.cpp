#include <cstdio>
#include <cstring>
#include <chrono>
#include <charconv>
#include <thread>
#include <string>

void sleep_seconds(double seconds) {
    std::chrono::duration<double> duration(seconds);
    std::this_thread::sleep_for(duration);
}

int main(int argc, char** argv) {
    bool print_space = false;
    if (argc != 2)
        return 1;
    double seconds = std::stod(argv[1]);
    sleep_seconds(seconds);
    return 0;
}