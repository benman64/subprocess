#include "shell_utils.hpp"

#include "basic_types.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <spawn.h>
#ifdef __APPLE__
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#include <errno.h>
#endif

#include <cctype>
#include <stdlib.h>
#include <map>
#include <mutex>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "ProcessBuilder.hpp"
using std::isspace;

namespace subprocess {
    std::string getcwd() {
        return std::filesystem::current_path().string();
    }
    void setcwd(const std::string& path) {
        std::filesystem::current_path(path);
    }
}
namespace {
    // this repititon exists here as I want shell to be fully standalone to
    // use in other projects
    bool is_file(const std::string &path) {
        try {
            if (path.empty())
                return false;
            return std::filesystem::is_regular_file(path);
        } catch (std::filesystem::filesystem_error& e) {
            return false;
        }
    }

    std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> result;
        std::stringstream ss (s);
        std::string item;

        while (getline(ss, item, delim)) {
            result.push_back (item);
        }

        return result;
    }
#ifdef _WIN32
    bool is_drive(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
#endif
    bool is_absolute_path(const std::string& path) {
        if(path.empty())
            return false;
#ifdef _WIN32
        if(is_drive(path[0]) && path[1] == ':')
            return true;
        return false;
#else
        if(path[0] == '/')
            return true;
        return false;
#endif
    }


    std::string clean_path(std::string path) {
        for(std::size_t i = 0; i < path.size(); ++i) {
            if(path[i] == '\\')
                path[i] = '/';
        }
#ifdef _WIN32
        if(path.size() == 2) {
            if(is_drive(path[0]) && path[1] == ':')
                path += '/';
        }
#endif
        while(path.size() >= 2 && path[path.size()-1] == '/' && path[path.size()-2] == '/')
            path.resize(path.size()-1);
        return path;
    }
    std::string join_path(std::string parent, std::string child) {
        if(child.empty())
            return parent;
        if(child == ".")
            return parent;
        if(child.find(':') != std::string::npos) {
            // if child has a ":" it is probably programmer error
        }
        parent = clean_path(parent);
        child = clean_path(child);
        while(child.size() >= 2) {
            if(child[0] == '.' && child[1] == '/') {
                child = child.substr(2);
            } else {
                break;
            }
        }
        if(parent[parent.size()-1] == '/') {
            if(child[0] == '/')
                parent += child.substr(1);
            else if(child[0] == '.' && child[1] == '/')
                parent += child.substr(2);
            else
                parent += child;
            return parent;
        }
        if(child[0] == '/')
            parent += child;
        else if(child[0] == '.' && child[1] == '/') {
            parent += child.substr(1);
        } else {
            parent += '/';
            parent += child;
        }

        return parent;
    }

}
namespace subprocess {
    std::string abspath(std::string dir, std::string relativeTo) {
        dir = clean_path(dir);
        if(is_absolute_path(dir))
            return dir;
        if(relativeTo.empty())
            relativeTo = subprocess::getcwd();
        if(!is_absolute_path(relativeTo)) {
            relativeTo = join_path(subprocess::getcwd(), relativeTo);
        }
        return join_path(relativeTo, dir);
    }

    std::string getenv(const std::string& var) {
        const char* ptr = ::getenv(var.c_str());
        if(ptr == nullptr)
            return "";
        return ptr;
    }
    std::string try_exe(std::string path) {
#ifdef _WIN32
        std::string path_ext = getenv("PATHEXT");
        if(path_ext.empty())
            path_ext = "exe";
        if(is_file(path))
            return path;
        for(std::string ext : split(path_ext, kPathDelimiter)) {
            if(ext.empty())
                continue;
            std::string test_path = path + ext;
            if(is_file(test_path))
                return test_path;
        }
#else
        if(is_file(path))
            return path;
#endif
        return "";
    }

    static std::mutex g_program_cache_mutex;
    static std::map<std::string, std::string> g_program_cache;

    static std::string find_program_in_path(const std::string& name) {
        // because of the cache variable is static we do this to be thread safe
        std::unique_lock lock(g_program_cache_mutex);

        std::map<std::string, std::string>& cache = g_program_cache;
        if(name.empty())
            return "";
        if(name.size() >= 2) {
            if((name[0] == '.' && name[1] == '/') || name[0] == '/') {
                if(is_file(name)) {
                    return abspath(name);
                }
            }

            if(is_absolute_path(name) || (name[0] == '.' && name[1] == '/')) {
                if(std::string test = try_exe(name); !test.empty()) {
                    if(is_file(test)) {
                        return abspath(test);
                    }
                }
            }

        }

        std::map<std::string, std::string>::iterator it = cache.find(name);
        if(it != cache.end())
            return it->second;

        std::string path_env = getenv("PATH");
        for(std::string test : split(path_env, kPathDelimiter)) {
            if(test.empty())
                continue;
            test += '/';
            test += name;
            test = try_exe(test);
            if(!test.empty() && is_file(test)) {
                cache[name] = test;
                return test;
            }
        }
        return "";
    }

    static bool is_python3(std::string path) {
        CompletedProcess process = subprocess::run({path, "--version"}, RunBuilder()
            .cout(PipeOption::pipe)
            .cerr(PipeOption::cout)
        );
        /*
        CompletedProcess process = subprocess::RunBuilder({path, "--version"})
            .cout(PipeOption::pipe)
            .cerr(PipeOption::cout)
            .run();


        since c++20 we can do this
        CompletedProcess process = subprocess::run({path, "--version"}, {
            .cout = PipeOption::pipe,
            .cerr = PipeOption::cout
        });
        */
        for (size_t i = 0; i < process.cout.size()-1; ++i) {
            char ch = process.cout[i];
            if (ch >= '0' && ch <= '9') {
                if (ch == '3' && process.cout[i+1] == '.')
                    return true;
                return false;
            }
        }
        return false;
    }
    std::string find_program(const std::string& name) {
        if (name != "python3") {
            return find_program_in_path(name);
        }
        std::string result = find_program_in_path(name);
        if (!result.empty())
            return result;

        std::string path_env = getenv("PATH");
        for(std::string test : split(path_env, kPathDelimiter)) {
            if(test.empty())
                continue;
            test += '/';
            test += "python";
            test = try_exe(test);
            if(!test.empty() && is_file(test)) {
                if (is_python3(test))
                    return test;
            }
        }
        return "";
    }

    void find_program_clear_cache() {
        std::unique_lock<std::mutex> lock(g_program_cache_mutex);
        g_program_cache.clear();
    }
    std::string escape_shell_arg(std::string arg) {
        bool needs_quote = false;
        for(std::size_t i = 0; i < arg.size(); ++i) {
            // white list
            if(isalpha(arg[i]))
                continue;
            if(isdigit(arg[i]))
                continue;
            if(arg[i] == '.')
                continue;
            if(arg[i] == '_' || arg[i] == '-' || arg[i] == '+' || arg[i] == '/')
                continue;

            needs_quote = true;
            break;
        }
        if(!needs_quote)
            return arg;
        std::string result = "\"";
        for(unsigned int i = 0; i < arg.size(); ++i) {
            if(arg[i] == '\"' || arg[i] == '\\')
                result += '\\';
            result += arg[i];
        }
        result += "\"";

        return result;
    }

}