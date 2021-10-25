#include "environ.hpp"

#include <stdlib.h>
#include <string>

#include "utf8_to_utf16.hpp"
using std::to_string;

#if !defined(_DCRTIMP)     // Windows-specific RTL DLL import macro
#define _DCRTIMP
#endif

extern "C" _DCRTIMP char **environ;

namespace subprocess {
    Environ cenv;

    EnvironSetter::EnvironSetter(const std::string& name) {
        mName = name;
    }
    EnvironSetter::operator bool() const {
        if (mName.empty())
            return false;
        const char* value = ::getenv(mName.c_str());
        if (value == nullptr) return false;
        return !!*value;
    }
    std::string EnvironSetter::to_string() {
        const char *value = ::getenv(mName.c_str());
        return value? value : "" ;
    }
    EnvironSetter &EnvironSetter::operator=(const std::string &str) {
        return *this = str.c_str();
    }

    EnvironSetter &EnvironSetter::operator=(const char* str) {
        if (mName == "PATH" || mName == "Path" || mName == "path") {
            find_program_clear_cache();
        }
#ifdef _WIN32
        // if it's empty windows deletes it.
        _putenv_s(mName.c_str(), str? str : "");
#else
        if (str == nullptr || !*str) {
            unsetenv(mName.c_str());
        } else {
            setenv(mName.c_str(), str, true);
        }
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


    EnvGuard::~EnvGuard() {
        auto new_env = current_env_copy();
        for(auto& var : new_env) {
            auto it = mEnv.find(var.first);
            if (it == mEnv.end()) {
                cenv[var.first] = nullptr;
            } else if (var.second != it->second) {
                cenv[it->first] = it->second;
            }
        }
        for (auto& var : mEnv) {
            cenv[var.first] = var.second;
        }
    }
    EnvMap current_env_copy() {
        EnvMap env;
#ifdef _WIN32
        static_assert(sizeof(wchar_t) == 2, "unexpected size of wchar_t");
        wchar_t* env_block = GetEnvironmentStringsW();
        if (env_block == nullptr) {
            throw OSError("GetEnvironmentStringsW failed");
        }
        for (wchar_t* list = env_block; *list; list += strlen16(list)+1) {
            std::string u8str = utf16_to_utf8(list);
            const char *name_start = u8str.c_str();
            const char *name_end = name_start;
            while (*name_end != '=' && *name_end)++name_end;
            if (*name_end != '=' || name_end == name_start)
                continue;
            std::string name(name_start, name_end);
            const char *value = name_end+1;
            if (!*value)
                continue;
            env[name] = value;
        }
        FreeEnvironmentStringsW(env_block);
#else
        for (char** list = environ; *list; ++list) {
            char *name_start = *list;
            char *name_end = name_start;
            while (*name_end != '=' && *name_end)++name_end;
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

    std::u16string create_env_block(const EnvMap& map) {
        size_t size = 0;
        for (auto& pair : map) {
            size += pair.first.size() + pair.second.size() + 2;
        }
        size += 1;

        std::u16string result;
        result.reserve(size*2);
        for (auto& pair : map) {
            std::string line = pair.first + "=" + pair.second;
            result += utf8_to_utf16(line);
            result += (char16_t)'\0';
        }
        result += (char16_t)'\0';
        return result;
    }
}
