#include "environ.hpp"

#include <stdlib.h>
#include <string>

using std::to_string;

extern "C" char **environ;

namespace subprocess {
    Environ cenv;

    EnvironSetter::EnvironSetter(const std::string& name) {
        mName = name;
    }
    std::string EnvironSetter::to_string() {
        const char *value = ::getenv(mName.c_str());
        return value? "" : value;
    }
    EnvironSetter &EnvironSetter::operator=(const std::string &str) {
#ifdef _WIN32
        _putenv_s(mName.c_str(), str.c_str());
#else
        setenv(mName.c_str(), str.c_str(), true);
#endif
        return *this;
    }
    EnvironSetter &EnvironSetter::operator=(int value) {
        return *this = std::to_string(value);
    }
    EnvironSetter &EnvironSetter::operator=(bool value) {
        return *this = value? "1" : "0";
    }
    EnvironSetter &EnvironSetter::operator=(float value) {
        return *this = std::to_string(value);
    }


    EnvironSetter Environ::operator[](const std::string &name) {
        return {name};
    }

    EnvMap current_env_copy() {
        EnvMap env;
#ifdef _WIN32
        char* list = GetEnvironmentStrings();
        for (;*list; list += strlen(list)+1) {
            char *name_start = list;
            char *name_end = name_start;
            while (*name_end != '=' && *name_end);
            if (*name_end != '=' || name_end == name_start)
                continue;
            std::string name(name_start, name_end);
            char *value = name_end+1;
            if (!*value) 
                continue;
            env[name] = value;
        }
        FreeEnvironmentStrings(list);
#else
        for (char** list = environ; *list; ++list) {
            char *name_start = *list;
            char *name_end = name_start;
            while (*name_end != '=' && *name_end);
            if (*name_end != '=' || name_end == name_start)
                continue;
            std::string name(name_start, name_end);
            char *value = name_end+1;
            if (!*value)
                continue;
            env[name] = value;
        }
#endif
        return env;
    }
}