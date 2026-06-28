import argparse
import os
import subprocess
from PIL import Image

def resize_image(src, dst_temp, size):
    """Resizes the image using Pillow and saves it to a temporary PNG file."""
    print(f"[1/2] Resizing image to {size}x{size}...")
    with Image.open(src) as img:
        # Convert to RGBA mode to ensure compatibility with alpha channels
        if img.mode != 'RGBA':
            img = img.convert('RGBA')
        
        # High-quality resize using Lanczos resampling filter
        resized_img = img.resize(size, Image.Resampling.LANCZOS)
        resized_img.save(dst_temp, "PNG")

def compress_astc(src_png, dst_astc, block_size):
    """Compresses the temporary PNG into ASTC format using astcenc CLI."""
    print(f"[2/2] Compressing to ASTC (Block size: {block_size})...")
    # Default speed preset is set to -medium. You can change it if needed.
    cmd = ["astcenc", "-cl", src_png, dst_astc, block_size, "-medium"]
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

def compress_dxt(src_png, dst_dds, codec_format):
    """Compresses the temporary PNG into BC/DXT formats using CompressonatorCLI."""
    print(f"[2/2] Compressing to DXT/BC ({codec_format})...")
    cmd = ["../tools/compressonatorcli/compressonatorcli.exe", "-miplevels", "10", "-NumThreads", "16","-fd", codec_format, src_png, dst_dds]
    #cmd = ["../tools/compressonatorcli/compressonatorcli.exe"]
    cwd = os.getcwd()
    print(f"Current working directory: {cwd}")
    print(f"Current cmd: {cmd}")
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

def parse_size(size_str):
    """Parses size input strings like '512x512' or '512' into integer tuples."""
    try:
        if 'x' in size_str.lower():
            w, h = map(int, size_str.lower().split('x'))
            return w, h
        else:
            val = int(size_str)
            return val, val
    except ValueError:
        raise argparse.ArgumentTypeError(
            "Size must be in 'WIDTHxHEIGHT' format (e.g., 512x512) or a single integer (e.g., 512)."
        )

def main():
    parser = argparse.ArgumentParser(description="Resize and compress textures into ASTC or BC (DXT) formats.")
    
    # Supported compression profiles
    choices_methods = ["astc4", "astc6", "astc8", "bc3", "bc6h", "bc7"]
    
    # Define required arguments
    parser.add_argument("src", help="Path to the source image file")
    parser.add_argument("dst", help="Path to save the final compressed file")
    parser.add_argument("size", type=parse_size, help="Target size. Format: 512x512 or 512 (for uniform square)")
    parser.add_argument("method", choices=choices_methods, help=f"Compression method. Options: {', '.join(choices_methods)}")

    args = parser.parse_args()

    if not os.path.exists(args.src):
        print(f"Error: Source file '{args.src}' not found.")
        return

    # Create destination directory if it does not exist
    dst_dir = os.path.dirname(args.dst)
    if dst_dir:
        os.makedirs(dst_dir, exist_ok=True)

    # Generate path for the temporary PNG file
    temp_png = os.path.join(dst_dir, f"temp_resize_{os.path.basename(args.src)}.png")

    try:
        # Step 1: Perform the resize operation
        resize_image(args.src, temp_png, args.size)
        
        # Step 2: Pass the resized file to the chosen compression backend
        if args.method.startswith("astc"):
            # Map user argument to astcenc block dimensions
            block_mapping = {"astc4": "4x4", "astc6": "6x6", "astc8": "8x8"}
            block_size = block_mapping[args.method]
            result = compress_astc(temp_png, args.dst, block_size)
        else:
            # Map user argument to Compressonator codec format flag (requires upper-case)
            codec_mapping = {"bc3": "BC3", "bc6h": "BC6H", "bc7": "BC7"}
            codec_format = codec_mapping[args.method]
            result = compress_dxt(temp_png, args.dst, codec_format)
            
        # Check CLI execution output status
        if result.returncode == 0:
            print(f"[Success] File successfully saved to: {args.dst}")
        else:
            print(f"[Compressor Error]:\n{result.stderr}\n{result.stdout}")
            
    except Exception as e:
        print(f"An error occurred during processing: {e}")
        
    finally:
        # Clean up the temporary file to free disk space
        if os.path.exists(temp_png):
            os.remove(temp_png)

if __name__ == "__main__":
    main()
