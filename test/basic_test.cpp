#include <cxxtest/TestSuite.h>

#include <subprocess.hpp>

using subprocess::CommandLine;
using subprocess::CompletedProcess;
using subprocess::PipeOption;
using subprocess::PopenBuilder;

bool is_equal(const CommandLine& a, const CommandLine& b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

class BasicSuite : public CxxTest::TestSuite {
public:
    void testFindProgram() {
        std::string path = subprocess::find_program("echo");
        TS_ASSERT(!path.empty());
    }
    void testHelloWorld() {
        CompletedProcess completed = subprocess::run({"echo", "hello", "world"},
            PopenBuilder().cout(PipeOption::pipe));
        TS_ASSERT_EQUALS(completed.cout, "hello world\n");
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::run({"echo", "hello", "world"}, PopenBuilder()
            .cout(PipeOption::cerr)
            .cerr(PipeOption::pipe));

        TS_ASSERT_EQUALS(completed.cerr, "hello world\n");
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);

    }

    void testHelloWorldBuilder() {
        CompletedProcess completed = subprocess::RunBuilder({"echo", "hello", "world"})
            .cout(PipeOption::pipe)
            .run();
        TS_ASSERT_EQUALS(completed.cout, "hello world\n");
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::RunBuilder({"echo", "hello", "world"})
            .cout(PipeOption::cerr)
            .cerr(PipeOption::pipe).run();

        TS_ASSERT_EQUALS(completed.cerr, "hello world\n");
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);

    }

    void testHelloWorld20() {
        std::string message = "__cplusplus = " + std::to_string(__cplusplus);
        TS_TRACE(message);
        // gcc 9 as installed by ubuntu doesn't have -std=c++20. So we use
        // this experimental value.
        #if __cplusplus == 201707L
        TS_TRACE("using C++20");
        CompletedProcess completed = subprocess::run({"echo", "hello", "world"}, {
            .cout = PipeOption::pipe
        });
        TS_ASSERT_EQUALS(completed.cout, "hello world\n");
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::run({"echo", "hello", "world"}, {
            .cout = PipeOption::cerr,
            .cerr = PipeOption::pipe
        });
        TS_ASSERT_EQUALS(completed.cerr, "hello world\n");
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);
        #else
        TS_SKIP("not c++20");
        #endif
    }
/*
    void tesxtCat() {
        CompletedProcess completed = subprocess::run({"cat"},
            PopenBuilder().cout(PipeOption::pipe)
            .build());
        

    }
*/
};


