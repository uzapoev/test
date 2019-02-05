#!/usr/bin/env python

# /*
# (c) 2014 +++ Filip Stoklas, aka FipS, http://www.4FipS.com +++
# THIS CODE IS FREE - LICENSED UNDER THE MIT LICENSE
# ARTICLE URL: http://forums.4fips.com/viewtopic.php?f=3&t=1201
# */

import os
import stat
from shutil import rmtree
from subprocess import check_call

def resolve_path(rel_path):
    return os.path.abspath(os.path.join(os.path.dirname(__file__), rel_path)) 

def rmtree_silent(root):
    def remove_readonly_handler(fn, root, excinfo):
        if fn is os.rmdir:
            if os.path.isdir(root): # if exists
                os.chmod(root, stat.S_IWRITE) # make writable
                os.rmdir(root)
        elif fn is os.remove:
            if os.path.isfile(root): # if exists
                os.chmod(root, stat.S_IWRITE) # make writable
                os.remove(root)
    rmtree(root, onerror=remove_readonly_handler)

def makedirs_silent(root):
    try:
        os.makedirs(root)
    except OSError: # mute if exists
        pass

if __name__ == "__main__":
    text = "Select a type: " \
    "\n1) Visual Studio" \
    "\n2) MacOS" \
    "\n3) IOS" \
    "\n" 
    x = int(input(text))
    if x > 0 and x < 3:
        build_dir = resolve_path("./build0")
        rmtree_silent(build_dir)
        makedirs_silent(build_dir)
        os.chdir(build_dir)
        check_call(["cmake", ".."])
    
#    os.chdir(build_dir)
#
#    check_call([
#     "cmake",
#     os.path.expandvars("-DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake"),
#     "-DCMAKE_BUILD_TYPE=Release",
#     "-DCMAKE_MAKE_PROGRAM=mingw32-make",
#     "-G", "Unix Makefiles",
#     ".."
#    ])
#
#    check_call(["mingw32-make"])