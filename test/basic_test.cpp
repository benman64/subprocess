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
            PopenBuilder().cout(PipeOption::pipe).build());
        TS_ASSERT_EQUALS(completed.cout, "hello world\n");
        TS_ASSERT(completed.cerr.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        CommandLine args = {"hello", "world"};
        TS_ASSERT_EQUALS(completed.args, args);

        completed = subprocess::run({"echo", "hello", "world"}, PopenBuilder()
            .cout(PipeOption::cerr)
            .cerr(PipeOption::pipe).build());

        TS_ASSERT_EQUALS(completed.cerr, "hello world\n");
        TS_ASSERT(completed.cout.empty());
        TS_ASSERT_EQUALS(completed.returncode, 0);
        TS_ASSERT_EQUALS(completed.args, args);

    }

};


