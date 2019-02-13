import os
import stat
from shutil import copyfile
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

def filelist(path):
    flist = list()
    for dirname, dirnames, filenames in os.walk(path):
      for filename in filenames:
             filepath = os.path.join(dirname, filename)
             flist.append(filepath)
    return flist

if __name__ == "__main__":

    build_dir = resolve_path("./data")
    rmtree_silent(build_dir)
    makedirs_silent(build_dir)

    files = filelist('./art')
    for filepath in files:
        filename, file_extension = os.path.splitext(filepath)
        dst = filename + file_extension + ".spv"
        dst = dst.replace('./art', './data')
        directory = os.path.dirname(dst)
        makedirs_silent(directory)
        if file_extension == ".vert" or file_extension == ".frag":
            check_call(["glslangValidator", "-H -V", filepath, "-o", dst])
        dst = dst.replace('.spv', '')
        copyfile(filepath, dst)

        print(filename + "|" + file_extension);
    os.chdir(build_dir)