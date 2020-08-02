#pragma once

#include <string>


namespace subprocess {
    /** Get the value of environment variable */
    std::string getenv(const std::string& var);
    /** Searches for program in PATH environment variable.

        on windows tries adding suffixes specified in PATHEXT environment
        variable.

        on windows an input of "python3" will also search "python" executables
        and inspect their version to find an executable that offers python 3.x.
    */
    std::string find_program(const std::string& name);
    /** Clears cache used by find_program.

        find_program uses internal cache to find executables. If you modify PATH
        using subprocess::cenv this will be called automatically for you. If a
        new program is added to a folder with the same name as an existing program
        you may want to clear the cache so that the new program is found as
        expected instead of the old program being returned.
    */
    void find_program_clear_cache();
    /** Escapes the argument suitable for use on command line. */
    std::string escape_shell_arg(std::string arg);

    /** Gets the current working directory of the process */
    std::string getcwd();
    /** Sets the current working directory of process. */
    void setcwd(const std::string& path);

    /** Converts dir to be absolute path.

        @param dir          The path you want to be absolute.
        @param relativeTo   If specified use this instead of the current working
                            directory.

        @return the absolute path.
    */
    std::string abspath(std::string dir, std::string relativeTo="");
}