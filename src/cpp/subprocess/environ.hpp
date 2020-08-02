#pragma once

#include <string>
#include <vector>

#include "basic_types.hpp"
#include "shell_utils.hpp"

namespace subprocess {
    class EnvironSetter {
    public:
        EnvironSetter(const std::string& name);
        operator std::string() { return to_string(); }
        explicit operator bool() const;
        std::string to_string();
        EnvironSetter &operator=(const std::string &str);
        EnvironSetter &operator=(const char* str);
        EnvironSetter &operator=(std::nullptr_t) { return *this = (const char*)0;}
        EnvironSetter &operator=(int value);
        EnvironSetter &operator=(bool value);
        EnvironSetter &operator=(float value);
    private:
        std::string mName;
    };

    /** Used for working with environment variables */
    class Environ {
    public:
        EnvironSetter operator[](const std::string&);
    };

    /** Makes it easy to get/set environment variables.
        e.g. like so `subprocess::cenv["VAR"] = "Value";`
    */
    extern Environ cenv;

    /** Creates a copy of current environment variables and returns the map */
    EnvMap current_env_copy();
    /** Gives an environment block used in Windows APIs. Each item is null
        terminated, end of list is double null-terminated and conforms to
        expectations of various windows API.
    */
    std::u16string create_env_block(const EnvMap& map);

    /** Use this to put a guard for changing current working directory. On
        destruction the current working directory will be reset to as it was
        on construction.
    */
    class CwdGuard {
    public:
        CwdGuard() {
            mCwd = subprocess::getcwd();
        }
        ~CwdGuard() {
            subprocess::setcwd(mCwd);
        }

    private:
        std::string mCwd;
    };

    /** On destruction reset environment variables and current working directory
        to as it was on construction.
    */
    class EnvGuard : public CwdGuard {
    public:
        EnvGuard() {
            mEnv = current_env_copy();
        }
        ~EnvGuard();

    private:
        EnvMap mEnv;
    };


}