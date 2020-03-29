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
    class EnvironGuard : public CwdGuard {
    public:
        EnvironGuard() {
            mEnv = current_env_copy();
        }
        ~EnvironGuard() {
            for (auto& var : mEnv) {
                cenv[var.first] = var.second;
            }
        }

    private:
        EnvMap mEnv;
    };


}