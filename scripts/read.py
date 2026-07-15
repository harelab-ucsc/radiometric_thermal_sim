import numpy as np
import matplotlib.pyplot as plt
import sys

def verify_radiometric_data(filepath):
    """
    Load and verify radiometric .raw file
    """
    try:
        with open(filepath, 'rb') as f:
            # Read width and height
            width = np.fromfile(f, np.uint32, 1)[0]
            height = np.fromfile(f, np.uint32, 1)[0]
            
            # Read temperature data as float32
            data = np.fromfile(f, np.float32, width * height)
            
            if len(data) == 0:
                print(f"ERROR: No data in file {filepath}")
                return None
            
            # Reshape to image
            temp_image = data.reshape((height, width))
            
            # Print statistics
            print(f"File: {filepath}")
            print(f"Image size: {width}x{height}")
            print(f"Data type: float32 (radiometric)")
            print(f"Temperature range: {temp_image.min():.2f}K to {temp_image.max():.2f}K")
            print(f"Mean temperature: {temp_image.mean():.2f}K")
            print(f"Std deviation: {temp_image.std():.2f}K")
            print(f"Sample values:\n{temp_image[0:5, 0:5]}")
            
            return temp_image
    
    except Exception as e:
        print(f"ERROR reading {filepath}: {e}")
        return None

def visualize_thermal(temp_image, filename="thermal_visualization.png"):
    """
    Visualize the thermal image
    """
    plt.figure(figsize=(10, 8))
    plt.imshow(temp_image, cmap='hot')
    plt.colorbar(label='Temperature (K)')
    plt.title('Radiometric Thermal Image')
    plt.xlabel('X pixels')
    plt.ylabel('Y pixels')
    plt.tight_layout()
    plt.savefig(filename, dpi=100)
    print(f"Visualization saved to {filename}")
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python verify_radiometric.py <path_to_thermal_0.raw>")
        print("\nExample: python verify_radiometric.py ./thermal_radiometric/thermal_0.raw")
        sys.exit(1)
    
    filepath = sys.argv[1]
    temp_image = verify_radiometric_data(filepath)
    
    if temp_image is not None:
        # Ask if user wants visualization
        response = input("\nVisualize thermal image? (y/n): ").lower()
        if response == 'y':
            visualize_thermal(temp_image)