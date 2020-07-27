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