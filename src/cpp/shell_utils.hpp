#pragma once

#include <string>


namespace subprocess {
    std::string getenv(const std::string& var);
    std::string find_program(const std::string& name);
    std::string escape_shell_arg(std::string arg);
}