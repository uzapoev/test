import os, sys, stat, time
import subprocess
from PIL import Image
from pathlib import Path
from shutil import copyfile
from shutil import rmtree
from subprocess import check_call

compressonator = './utils/compressonatorcli/compressonatorcli.exe'
tmp_image_path = "./assets/temp/image"
dst_compressed_path = './assets/temp/image_compressed'
image_extensions = ['.jpg','.jpeg', '.bmp', '.png', '.gif', '.tga']

def pause():
    programPause = raw_input("Press the <ENTER> key to continue...")

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
        
        
def resize_textures(files, dst_dir):
     for path in files:
        filename = os.path.basename(path)       # filename with extension
        filename, extension = os.path.splitext(filename)
        dst_file_path = dst_dir + '/' + filename + '.png';
        if os.path.isfile(dst_file_path):
            continue
            
        if extension in image_extensions:
            imageOriginal = Image.open(path)
            width, height = imageOriginal.size
            print(path, width, height)
            if width > 1024 or height > 1024:
                imageResized = imageOriginal.resize((1024, 1024), Image.LANCZOS )
                imageResized.save(dst_file_path, 'PNG')
            else:
                imageOriginal.save(dst_file_path, 'PNG')


def compress_textures(compression_type, src_dir, dst_dir):
    result = subprocess.run([compressonator, "-fd", compression_type, "-fx", "DDS", "-ff", "PNG", "-miplevels", "10", src_dir, dst_dir])
    print(result)


def export_textures(filepathes):
    makedirs_silent(dst_compressed_path)
    resize_textures(filepathes, tmp_image_path);
    compress_textures("BC3", tmp_image_path, dst_compressed_path)
    ##result = subprocess.run([compressonator, "-fd", "BC3", "-fx", "DDS", "-ff", "PNG", "-miplevels", "10", tmp_image_path, dst_compressed_path])
    #print(result)
#  if e is '.exr'
#      subprocess.run([compressonator, "-fd", "BC7", "-fx", "DDS", "-ff", "PNG", "-miplevels", "10", tmp_image_path, dst_compressed_path])

def filelist(path):
    flist = list()
    for dirname, dirnames, filenames in os.walk(path):
      for filename in filenames:
             filepath = os.path.join(dirname, filename)
             flist.append(filepath)
    return flist
    
    
def export_mesh(path):
    print("export mesh:" + path)


if __name__ == "__main__":

    for i in range(0, len(sys.argv)):
        print(sys.argv[i])

    build_dir = resolve_path("./data")
 #   rmtree_silent(build_dir)
    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(tmp_image_path, exist_ok=True)
    
    texture_list = []
    mesh_list = []

    files = filelist('./assets')
    
    for dirname, dirnames, filenames in os.walk('./assets'):
      for filename in filenames:
        filepath = os.path.join(dirname, filename).replace('\\', '/')
        filename, extension = os.path.splitext(filepath)
        
        if tmp_image_path in filepath:
            print("!!!!!skip!!!!" + filepath)
            continue
        # textures
        if extension in image_extensions:
            texture_list.append(filepath)
        #meshes

    export_textures(texture_list)
    os.chdir(build_dir)
    os.system("pause")
