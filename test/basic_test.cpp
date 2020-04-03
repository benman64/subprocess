#include <cxxtest/TestSuite.h>
#include <thread>

#include <subprocess.hpp>

#include "test_config.h"

using subprocess::CommandLine;
using subprocess::CompletedProcess;
using subprocess::PipeOption;
using subprocess::RunBuilder;

#ifdef _WIN32
#define EOL "\r\n"
#else
#define EOL "\n"
#endif
bool is_equal(const CommandLine& a, const CommandLine& b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

void nop() {
    // cxxtest is a bit dumb. We need this to enable exceptions
    throw 32;
}

std::string dirname(std::string path) {
    size_t slash_pos = path.size();;
    for (int i = 0; i < path.size(); ++i) {
        if (path[i] == '/' || path[i] == '\\')
            slash_pos = i;
    }
    return path.substr(0, slash_pos);
}
class BasicSuite : public CxxTest::TestSuite {
public:
    static BasicSuite* createSuite() {
        return new BasicSuite();
    }
    static void destroySuite(BasicSuite* suite) {
        delete suite;
    }
    void testPath() {
        std::string path = subprocess::cenv["PATH"];
        TS_ASSERT(!path.empty());
    }

    void testEnvGuard() {
        using subprocess::cenv;
        std::string path = cenv["PATH"];
        std::string world = subprocess::cenv["HELLO"];
        TS_ASSERT_EQUALS(world, "");

        TS_TRACE("entering EnvGuard scope");
        {
            subprocess::EnvGuard guard;
            TS_TRACE("setting HELLO=world");
            subprocess::cenv["HELLO"] = "world";
            world = cenv["HELLO"];
            TS_ASSERT_EQUALS(world, "world");
            TS_TRACE("exiting EnvGuard scope");
        }
        world = cenv["HELLO"];
        TS_ASSERT_EQUALS(world, "");
        std::string new_path = cenv["PATH"];
        TS_ASSERT_EQUALS(path, new_path);
    }

    void testFindProgram() {
        std::string path = subprocess::find_program("echo");
        TS_ASSERT(!path.empty());
    }
    void testHelloWorld() {
        CompletedProcess completed = subprocess::run({"echo", "hello", "world"},
            RunBuilder().cout(PipeOption::pipe));
        TS_ASSERT_EQUALS(completed.cout, "hello world" EOL);
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::run({"echo", "hello", "world"}, RunBuilder()
            .cout(PipeOption::cerr)
            .cerr(PipeOption::pipe));

        TS_ASSERT_EQUALS(completed.cerr, "hello world" EOL);
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);

    }

    void testHelloWorldBuilder() {
        CompletedProcess completed = subprocess::RunBuilder({"echo", "hello", "world"})
            .cout(PipeOption::pipe)
            .run();
        TS_ASSERT_EQUALS(completed.cout, "hello world" EOL);
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::RunBuilder({"echo", "hello", "world"})
            .cout(PipeOption::cerr)
            .cerr(PipeOption::pipe).run();

        TS_ASSERT_EQUALS(completed.cerr, "hello world" EOL);
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);

    }

    void testHelloWorld20() {
        std::string message = "__cplusplus = " + std::to_string(__cplusplus);
        TS_TRACE(message);
        // gcc 9 as installed by ubuntu doesn't have -std=c++20. So we use
        // this experimental value.
        #if __cplusplus == 201707L && 0
        TS_TRACE("using C++20");
        CompletedProcess completed = subprocess::run({"echo", "hello", "world"}, {
            .cout = PipeOption::pipe
        });
        TS_ASSERT_EQUALS(completed.cout, "hello world" EOL);
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::run({"echo", "hello", "world"}, {
            .cout = PipeOption::cerr,
            .cerr = PipeOption::pipe
        });
        TS_ASSERT_EQUALS(completed.cerr, "hello world" EOL);
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);
        #else
        TS_SKIP("not c++20");
        #endif
    }

    void testNotFound() {
        TS_ASSERT_THROWS_ANYTHING(subprocess::run({"yay-322"}));
    }

    void testCin() {
        auto completed = subprocess::run({"cat"},
            RunBuilder().cin("hello world").cout(PipeOption::pipe));
        TS_ASSERT_EQUALS(completed.cout, "hello world");
    }

    void testNewEnvironment() {
        subprocess::EnvGuard guard;
        std::string path = subprocess::cenv["PATH"];
        path = TEST_BINARY_DIR + subprocess::kPathDelimiter + path;
        subprocess::cenv["PATH"] = path;

        subprocess::EnvMap env = subprocess::current_env_copy();
        TS_ASSERT_EQUALS(subprocess::cenv["HELLO"].to_string(), "");
        env["HELLO"] = "world";
        TS_ASSERT_EQUALS(subprocess::cenv["HELLO"].to_string(), "");

        auto completed = subprocess::RunBuilder({"printenv", "HELLO"})
            .cout(PipeOption::pipe).env(env).run();

        TS_ASSERT_EQUALS(completed.cout, "world" EOL);
    }

    void testSleep() {
        subprocess::StopWatch timer;
        subprocess::sleep_seconds(1);
        TS_ASSERT_DELTA(timer.seconds(), 1, 0.1);
    }
    void testCerrToCout() {
        subprocess::EnvGuard guard;
        std::string path = subprocess::cenv["PATH"];
        path = std::string() + TEST_BINARY_DIR + subprocess::kPathDelimiter + path;
        TS_TRACE("TEST_BINARY_DIR = " TEST_BINARY_DIR);

        TS_TRACE("setting PATH = " + path);
        subprocess::cenv["PATH"] = path;
        path = subprocess::cenv["PATH"];
        TS_TRACE("PATH = " + path);
        subprocess::cenv["USE_CERR"] = "1";
        subprocess::find_program_clear_cache();
        std::string echo_path = subprocess::find_program("echo");
        TS_TRACE("echo is at " + echo_path);
        // TOOD: test cerr to cout
        auto completed = RunBuilder({"echo", "hello", "world"})
            .cout(subprocess::PipeOption::pipe)
            .cerr(subprocess::PipeOption::pipe)
            .env(subprocess::current_env_copy())
            .run();
        TS_ASSERT_EQUALS(completed.cout, "");
        TS_ASSERT_EQUALS(completed.cerr, "hello world" EOL);

        completed = RunBuilder({"echo", "hello", "world"})
            .cerr(subprocess::PipeOption::cout)
            .cout(PipeOption::pipe).run();

        TS_ASSERT_EQUALS(completed.cout, "hello world" EOL);
        TS_ASSERT_EQUALS(completed.cerr, "");

    }

    void testCoutToCerr() {
        // cause we can
        subprocess::EnvGuard guard;
        std::string path = subprocess::cenv["PATH"];
        path = std::string() + TEST_BINARY_DIR + subprocess::kPathDelimiter + path;
        TS_TRACE("TEST_BINARY_DIR = " TEST_BINARY_DIR);

        TS_TRACE("setting PATH = " + path);
        subprocess::cenv["PATH"] = path;

        auto completed = RunBuilder({"echo", "hello", "world"})
            .cerr(subprocess::PipeOption::pipe)
            .cout(PipeOption::cerr).run();

        TS_ASSERT_EQUALS(completed.cout, "");
        TS_ASSERT_EQUALS(completed.cerr, "hello world" EOL);
    }

    void testPoll() {

    }

    void testWaitTimeout() {
        subprocess::EnvGuard guard;
        std::string path = subprocess::cenv["PATH"];
        path = TEST_BINARY_DIR + subprocess::kPathDelimiter + path;
        subprocess::cenv["PATH"] = path;

        auto popen = RunBuilder({"sleep", "10"}).popen();
        subprocess::StopWatch timer;

        TS_ASSERT_THROWS(popen.wait(3), subprocess::TimeoutExpired)

        popen.terminate();
        popen.close();

        double timeout = timer.seconds();
        TS_ASSERT_DELTA(timeout, 3, 0.1);

    }

    void test2ProcessConnect() {

    }

    void testKill() {
        subprocess::EnvGuard guard;
        std::string path = subprocess::cenv["PATH"];
        path = TEST_BINARY_DIR + subprocess::kPathDelimiter + path;
        subprocess::cenv["PATH"] = path;

        auto popen = RunBuilder({"sleep", "10"}).popen();
        subprocess::StopWatch timer;
        std::thread thread([&] {
            subprocess::sleep_seconds(3);
            popen.kill();
        });

        thread.detach();

        popen.close();

        double timeout = timer.seconds();
        TS_ASSERT_DELTA(timeout, 3, 0.1);
    }

    void testTerminate() {
        subprocess::EnvGuard guard;
        std::string path = subprocess::cenv["PATH"];
        path = TEST_BINARY_DIR + subprocess::kPathDelimiter + path;
        subprocess::cenv["PATH"] = path;

        auto popen = RunBuilder({"sleep", "10"}).popen();
        subprocess::StopWatch timer;
        std::thread thread([&] {
            subprocess::sleep_seconds(3);
            popen.terminate();
        });

        thread.detach();

        popen.close();

        double timeout = timer.seconds();
        TS_ASSERT_DELTA(timeout, 3, 0.1);
    }

    void testSIGINT() {
        auto popen = RunBuilder({"sleep", "10"}).popen();
        subprocess::StopWatch timer;
        std::thread thread([&] {
            subprocess::sleep_seconds(3);
            popen.send_signal(subprocess::SigNum::PSIGINT);
        });

        thread.detach();

        popen.close();

        double timeout = timer.seconds();
        TS_ASSERT_DELTA(timeout, 3, 0.1);

    }


/*
    void tesxtCat() {
        CompletedProcess completed = subprocess::run({"cat"},
            PopenBuilder().cout(PipeOption::pipe)
            .build());
        

    }
*/

    /*  Tests TODO: write tests

        connect to apps together.
        check=true
        RunBuilder using popen()
        program not found

    */
};


int main(int argc, char** argv) {
    using subprocess::abspath;
    std::string path = subprocess::cenv["PATH"];
    std::string more_path = dirname(abspath(argv[0]));
    path += subprocess::kPathDelimiter;
    path += more_path;
    subprocess::cenv["PATH"] = path;

    return cxxtest_main(argc, argv);
}