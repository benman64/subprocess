#!/usr/bin/env python3
import os
import subprocess
import sys

def system(*args, cwd=None):
    cmd = " ".join(args)
    print(f"> {cmd}")
    sys.stderr.flush()
    sys.stdout.flush()
    subprocess.run(args, cwd=cwd, check=True)

def mkdir_if_needed(path):
    if not os.path.exists(path):
        os.mkdir(path)

def build(path):
    system("cmake", "--build", path)

mkdir_if_needed("build")
mkdir_if_needed("build/host")
mkdir_if_needed("build/ming")
mkdir_if_needed("build/ming-unicode")

system(
    "cmake", "-GNinja", "../../",
    "-DBUILD_TESTING=1",
    cwd="build/host"
)
system(
    "cmake", "-GNinja",
    "../../",
    "-DBUILD_TESTING=1",
    "-DCMAKE_TOOLCHAIN_FILE=../../toolchain-x86_64-w64-mingw32.cmake",
    cwd="build/ming"
)
system(
    "cmake", "-GNinja",
    "../../",
    "-DBUILD_TESTING=1",
    "-DUNICODE=1",
    "-DCMAKE_TOOLCHAIN_FILE=../../toolchain-x86_64-w64-mingw32.cmake",
    cwd="build/ming-unicode"
)

build("build/host")
build("build/ming")
build("build/ming-unicode")

system("ninja", "test", cwd="build/host")
system("ninja", "test", cwd="build/ming")
system("ninja", "test", cwd="build/ming-unicode")