import base64
import re
import subprocess
import imageio.v3 as iio
import numpy as np

def capture_tiff(topic_name, output_path):
    """Capture a single frame from a gz topic and save as .tiff."""
    proc = subprocess.Popen(
        ["gz", "topic", "-e", "-t", topic_name],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    data_lines = []
    for line in proc.stdout:
        data_lines.append(line)
        if "data:" in line:  # stop after we hit data field
            break
    proc.kill()

    # Extract width, height, pixel_format, and base64 data
    full_data = "".join(data_lines)
    width = int(re.search(r"width:\s*(\d+)", full_data).group(1))
    height = int(re.search(r"height:\s*(\d+)", full_data).group(1))
    pixel_format = re.search(r"pixel_format_type:\s*\"([A-Z0-9_]+)\"", full_data).group(1)
    data_match = re.search(r"data:\s*\"([A-Za-z0-9+/=\n\r]+)\"", full_data)

    if not data_match:
        print(f"[WARN] No image data found for {topic_name}")
        return

    # Decode base64 image data
    img_bytes = base64.b64decode(data_match.group(1))
    img = np.frombuffer(img_bytes, dtype=np.uint8)

    if "RGB" in pixel_format:
        img = img.reshape(height, width, 3)
    else:
        img = img.reshape(height, width)

    iio.imwrite(output_path, img)
    print(f"[SAVED] {output_path} ({pixel_format}, {width}x{height})")
