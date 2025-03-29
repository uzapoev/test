import os, sys, stat, time
import subprocess
import shutil
from PIL import Image
from pathlib import Path
from shutil import copyfile
from shutil import rmtree
from subprocess import check_call

compressonator = './utils/compressonatorcli/compressonatorcli.exe'
tmp_image_path = "./assets/temp/image"
dst_compressed_path = './assets/temp/image_compressed/bc3'
dst_lm_compressed_path = './assets/temp/image_compressed/bc6'
image_extensions = ['.jpg','.jpeg', '.bmp', '.png', '.gif', '.tga']
lightmap_extensions = ['.exr']

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
        
def clear_dir(root):
    rmtree_silent(root)
    makedirs_silent(root)
        
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
            if width > 512 or height > 512:
                imageResized = imageOriginal.resize((512, 512), Image.LANCZOS )
                imageResized.save(dst_file_path, 'PNG')
            else:
                imageOriginal.save(dst_file_path, 'PNG')


def compress_textures(compression_type, src_dir, dst_dir):
    result = subprocess.run([compressonator, "-fd", compression_type, "-fx", "DDS", "-ff", "PNG", "-miplevels", "10", src_dir, dst_dir])
    print(result)
    
def compress_lighmaps(compression_type, src_dir, dst_dir):
    result = subprocess.run([compressonator, "-fd", compression_type, "-fx", "DDS", "-ff", "exr", "-miplevels", "10", src_dir, dst_dir])
    print(result)


def export_textures(filepathes, dst_path, pixelformat):
    makedirs_silent(dst_path)
    compress_textures(pixelformat, tmp_image_path, dst_path)


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
    
    clear_dir(tmp_image_path);
    
    texture_list = []
    lightmap_list = []
    mesh_list = []

    files = filelist('./assets')
    
    for dirname, dirnames, filenames in os.walk('./assets'):
      for filename in filenames:
        filepath = os.path.join(dirname, filename).replace('\\', '/')
        filename, extension = os.path.splitext(filepath)
        
        if tmp_image_path in filepath:
            #print("!!!!!skip!!!!" + filepath)
            continue
        # textures
        if extension in image_extensions:
            texture_list.append(filepath)
            
        if extension in lightmap_extensions:
            lightmap_list.append(filepath)
        #meshes

    clear_dir(tmp_image_path)

    resize_textures(texture_list, tmp_image_path)
    makedirs_silent(dst_compressed_path)
    compress_textures("BC3", tmp_image_path, dst_compressed_path)
    clear_dir(tmp_image_path)
    
    for filepath in lightmap_list:
        print("!!!!!lm!!!!" + filepath)
        shutil.copy2(filepath, tmp_image_path) 
    makedirs_silent(dst_lm_compressed_path)
    compress_lighmaps("BC6H", tmp_image_path, dst_lm_compressed_path)
   # clear_dir(tmp_image_path)
   # export_textures(lightmap_list, dst_lm_compressed_path, "BC6")
    
    os.chdir(build_dir)
    os.system("pause")
