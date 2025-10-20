#!/usr/bin/env python3
"""
Convert thermal PNG files saved by Gazebo to radiometric float32 format.
This bypasses the broken topic publishing and uses the correctly-rendered PNG saves.
"""

import numpy as np
from PIL import Image
import struct
import glob
import os
import sys

def convert_thermal_png_to_radiometric(png_path, output_path, min_temp=253.15, max_temp=373.0):
    """
    Convert a 16-bit thermal PNG to radiometric float32 format.
    
    Args:
        png_path: Path to input PNG file
        output_path: Path to output .raw file
        min_temp: Minimum temperature in Kelvin
        max_temp: Maximum temperature in Kelvin
    """
    # Read the PNG file
    img = Image.open(png_path)
    
    # Convert to numpy array
    # Gazebo saves thermal images as 16-bit grayscale
    thermal_data = np.array(img, dtype=np.uint16)
    
    height, width = thermal_data.shape
    
    print(f"Processing: {png_path}")
    print(f"  Size: {width}x{height}")
    print(f"  Raw uint16 range: [{thermal_data.min()}, {thermal_data.max()}]")
    
    # Convert to float32 temperature values
    # Normalize uint16 (0-65535) to temperature range
    normalized = thermal_data.astype(np.float32) / 65535.0
    temp_data = min_temp + (normalized * (max_temp - min_temp))
    
    print(f"  Temperature range: [{temp_data.min():.2f}K, {temp_data.max():.2f}K]")
    print(f"  Mean temperature: {temp_data.mean():.2f}K")
    
    # Write to binary file with header
    with open(output_path, 'wb') as f:
        # Write header: width, height (as unsigned ints)
        f.write(struct.pack('II', width, height))
        
        # Write float32 temperature data
        temp_data.tofile(f)
    
    print(f"  Saved to: {output_path}")
    return True

def batch_convert(input_dir, output_dir, min_temp=253.15, max_temp=373.0):
    """
    Convert all thermal PNG files in a directory.
    
    Args:
        input_dir: Directory containing thermal PNG files
        output_dir: Directory to save radiometric .raw files
        min_temp: Minimum temperature in Kelvin
        max_temp: Maximum temperature in Kelvin
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Find all PNG files
    png_files = sorted(glob.glob(os.path.join(input_dir, "*.png")))
    
    if not png_files:
        print(f"No PNG files found in {input_dir}")
        return
    
    print(f"Found {len(png_files)} PNG files to convert")
    print(f"Temperature range: {min_temp}K to {max_temp}K")
    print("-" * 60)
    
    for png_path in png_files:
        # Generate output filename
        basename = os.path.splitext(os.path.basename(png_path))[0]
        output_path = os.path.join(output_dir, f"{basename}.raw")
        
        try:
            convert_thermal_png_to_radiometric(png_path, output_path, min_temp, max_temp)
            print()
        except Exception as e:
            print(f"ERROR processing {png_path}: {e}")
            print()
    
    print("-" * 60)
    print(f"Conversion complete! {len(png_files)} files processed.")
    print(f"Output directory: {output_dir}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage:")
        print("  Single file: python convert_thermal_png_to_radiometric.py <input.png> <output.raw> [min_temp] [max_temp]")
        print("  Batch mode:  python convert_thermal_png_to_radiometric.py <input_dir> <output_dir> [min_temp] [max_temp]")
        print()
        print("Default temperature range: 253.15K to 373.0K")
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    # Optional temperature parameters
    min_temp = float(sys.argv[3]) if len(sys.argv) > 3 else 253.15
    max_temp = float(sys.argv[4]) if len(sys.argv) > 4 else 373.0
    
    if os.path.isdir(input_path):
        # Batch mode
        batch_convert(input_path, output_path, min_temp, max_temp)
    else:
        # Single file mode
        if not input_path.endswith('.png'):
            print("ERROR: Input file must be a PNG file")
            sys.exit(1)
        
        convert_thermal_png_to_radiometric(input_path, output_path, min_temp, max_temp)