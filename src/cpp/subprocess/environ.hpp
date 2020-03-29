#pragma once

#include <string>
#include <vector>

#include "basic_types.hpp"

namespace subprocess {
    class EnvironSetter {
    public:
        EnvironSetter(const std::string& name);
        operator std::string() { return to_string(); }
        std::string to_string();
        EnvironSetter &operator=(const std::string &str);
        EnvironSetter &operator=(int value);
        EnvironSetter &operator=(bool value);
        EnvironSetter &operator=(float value);
    private:
        std::string mName;
    };
    class Environ {
    public:
        EnvironSetter operator[](const std::string&);
    };

    extern Environ cenv;

    /** Creates a copy of current environment variables and returns the map */
    EnvMap current_env_copy();
}