#!/usr/bin/python
from PIL import Image
from pathlib import Path
import os, sys
import subprocess
import time

n = len(sys.argv)
print("\nArguments passed:", end = " ")
for i in range(0, n):
    print(sys.argv[i], end = " ")

src_path = "./assets/unity/textures/"
dst_path = "./assets/unity/textures_dds/"
tmp_path = "./assets/unity/tmp/"

compressonator = './utils/compressonatorcli/compressonatorcli.exe'
image_extensions = ['.jpg','.jpeg', '.bmp', '.png', '.gif', '.tga']

dirs = os.listdir( src_path )

def resize():
    os.makedirs(dst_path, exist_ok=True)
    os.makedirs(tmp_path, exist_ok=True)
    for item in dirs:
        if os.path.isfile(src_path+item):
            filename, extension = os.path.splitext(tmp_path+item)
            if extension in image_extensions:
                im = Image.open(src_path+item)
                width, height = im.size
                if width > 1024 or height > 1024:
                    imResize = im.resize((1024, 1024), Image.LANCZOS )
                    imResize.save(filename + '.png', 'PNG')
                else:
                    im.save(filename + '.png', 'PNG')
                
                src_file = filename + '.png'
                dst_file = dst_path + os.path.basename(filename) + '.dds'
                result = subprocess.run([compressonator, "-fd", "BC3", "-fx", "DDS", "-miplevels", "10", src_file, dst_file])
                print(result)

          #  if extension is '.exr'
          #      im = Image.open(src_path+item)
          #      imResize = im.resize((1024,1024), Image.LANCZOS )
          #      imResize.save(filename + '.png', 'PNG')
          #      result = subprocess.run([compressonator, "-fd", "BC7", "-fx", "DDS", "-miplevels", "10", arg2, arg3])

resize()