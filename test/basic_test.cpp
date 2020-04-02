#include <cxxtest/TestSuite.h>

#include <subprocess.hpp>

using subprocess::CommandLine;
using subprocess::CompletedProcess;
using subprocess::PipeOption;
using subprocess::PopenBuilder;
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

        completed = subprocess::run({"echo", "hello", "world"}, PopenBuilder()
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
        auto completed = subprocess::capture({"cat"},
            PopenBuilder().cin("hello world"));
        TS_ASSERT_EQUALS(completed.cout, "hello world");
    }

    void testNewEnvironment() {

    }

    void testCerrToCout() {

    }

    void testCoutToCerr() {
        // cause we can
    }

    void testPoll() {

    }

    void testWaitTimeout() {

    }

    void test2ProcessConnect() {

    }

    void testKill() {

    }

    void testTerminate() {

    }

    void testSIGINT() {

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