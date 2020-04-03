#pragma once

#include <string>


namespace subprocess {
    std::string getenv(const std::string& var);
    std::string find_program(const std::string& name);
    void find_program_clear_cache();
    std::string escape_shell_arg(std::string arg);

    std::string getcwd();
    void setcwd(const std::string& path);

    std::string abspath(std::string dir, std::string relativeTo="");
}