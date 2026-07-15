#!/usr/bin/env python3
"""
Convert thermal PNG files saved by Gazebo to radiometric float32 format.

The PNGs must be saved with the thermal camera in L16 (16-bit) format, and
min_temp/max_temp/resolution here must match the ThermalSensor plugin
configured on that camera in the .sdf world file -- see thermalRandy.sdf.
"""

import numpy as np
from PIL import Image
import struct
import glob
import os
import sys

def convert_thermal_png_to_radiometric(png_path, output_path, resolution=0.01):
    """
    Convert a 16-bit thermal PNG to radiometric float32 format.

    gz-sim's thermal camera encodes raw = clamp(temp, min_temp, max_temp) / resolution,
    i.e. temp = raw * resolution (min_temp/max_temp only clamp the input
    temperature -- they are not part of the conversion itself). resolution
    MUST match the value configured on the thermal camera's ThermalSensor
    plugin in the .sdf world file, since that is what actually encoded the
    raw pixel values. Default 0.01 K/count matches gz-sim's own default for
    a 16-bit (L16) thermal camera.

    Args:
        png_path: Path to input PNG file
        output_path: Path to output .raw file
        resolution: Kelvin per raw uint16 count.
    """
    # Read the PNG file
    img = Image.open(png_path)

    # Gazebo's L16 thermal format saves as a genuine 16-bit grayscale PNG
    # (PIL mode "I" or "I;16"). An L8 (8-bit) source would load fine here but
    # silently produce near-zero, wrong temperatures once decoded as if it
    # were 16-bit -- so refuse it instead of guessing.
    if img.mode not in ("I", "I;16", "I;16B"):
        raise ValueError(
            f"{png_path}: expected a 16-bit grayscale PNG (mode 'I' or "
            f"'I;16'), got mode '{img.mode}'. Check the sensor's "
            f"<image><format> in the .sdf world file is L16, not L8."
        )

    thermal_data = np.array(img, dtype=np.uint16)

    height, width = thermal_data.shape

    print(f"Processing: {png_path}")
    print(f"  Size: {width}x{height}")
    print(f"  Raw uint16 range: [{thermal_data.min()}, {thermal_data.max()}]")

    # Convert to float32 temperature values using the same linear mapping
    # the ThermalSensor plugin uses to encode: temp = raw * resolution
    temp_data = thermal_data.astype(np.float32) * resolution

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

def batch_convert(input_dir, output_dir, resolution=0.01):
    """
    Convert all thermal PNG files in a directory.

    Args:
        input_dir: Directory containing thermal PNG files
        output_dir: Directory to save radiometric .raw files
        resolution: Kelvin per raw uint16 count (see convert_thermal_png_to_radiometric)
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)

    # Find all PNG files
    png_files = sorted(glob.glob(os.path.join(input_dir, "*.png")))

    if not png_files:
        print(f"No PNG files found in {input_dir}")
        return

    print(f"Found {len(png_files)} PNG files to convert")
    print(f"Resolution: {resolution} K/count")
    print("-" * 60)

    for png_path in png_files:
        # Generate output filename
        basename = os.path.splitext(os.path.basename(png_path))[0]
        output_path = os.path.join(output_dir, f"{basename}.raw")

        try:
            convert_thermal_png_to_radiometric(png_path, output_path, resolution)
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
        print("  Single file: python png_to_radiometric.py <input.png> <output.raw> [resolution]")
        print("  Batch mode:  python png_to_radiometric.py <input_dir> <output_dir> [resolution]")
        print()
        print("Default resolution: 0.01 K/count (gz-sim's default for L16)")
        print("resolution MUST match the ThermalSensor plugin in the .sdf world file.")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    resolution = float(sys.argv[3]) if len(sys.argv) > 3 else 0.01

    if os.path.isdir(input_path):
        # Batch mode
        batch_convert(input_path, output_path, resolution)
    else:
        # Single file mode
        if not input_path.endswith('.png'):
            print("ERROR: Input file must be a PNG file")
            sys.exit(1)

        convert_thermal_png_to_radiometric(input_path, output_path, resolution)