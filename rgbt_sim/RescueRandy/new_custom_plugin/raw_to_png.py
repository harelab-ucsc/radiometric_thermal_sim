#!/usr/bin/env python3
"""
Subscribe to thermal camera topic and save images properly
"""

import cv2
import numpy as np
from gz.msgs10.image_pb2 import Image
from gz.transport13 import Node
import os
from datetime import datetime

class ThermalImageSaver:
    def __init__(self, save_path, topic="/thermal/image"):
        self.save_path = save_path
        self.topic = topic
        self.frame_count = 0
        
        # Create save directory if it doesn't exist
        os.makedirs(save_path, exist_ok=True)
        
        # Initialize Gazebo transport node
        self.node = Node()
        
        # Subscribe to the thermal image topic
        if not self.node.subscribe(Image, topic, self.image_callback):
            print(f"Error subscribing to topic: {topic}")
            return
        
        print(f"Subscribed to {topic}")
        print(f"Saving images to: {save_path}")
        print("Press Ctrl+C to stop")
    
    def image_callback(self, msg):
        """Callback function for thermal images"""
        try:
            # Get image dimensions
            width = msg.width
            height = msg.height
            
            # Convert image data based on format
            if msg.pixel_format_type == Image.L_INT8:
                # 8-bit grayscale
                img_array = np.frombuffer(msg.data, dtype=np.uint8)
                img = img_array.reshape((height, width))
            elif msg.pixel_format_type == Image.L_INT16:
                # 16-bit grayscale - convert to 8-bit for saving
                img_array = np.frombuffer(msg.data, dtype=np.uint16)
                img = img_array.reshape((height, width))
                # Normalize to 8-bit range
                img = ((img - img.min()) / (img.max() - img.min()) * 255).astype(np.uint8)
            else:
                print(f"Unsupported pixel format: {msg.pixel_format_type}")
                return
            
            # Save image
            filename = os.path.join(self.save_path, f"thermal_{self.frame_count:06d}.png")
            cv2.imwrite(filename, img)
            
            if self.frame_count % 10 == 0:
                print(f"Saved frame {self.frame_count}: {filename}")
            
            self.frame_count += 1
            
        except Exception as e:
            print(f"Error processing image: {e}")
    
    def spin(self):
        """Keep the node running"""
        print("\nWaiting for images...")
        try:
            while True:
                pass
        except KeyboardInterrupt:
            print(f"\nStopping... Saved {self.frame_count} frames total")

if __name__ == "__main__":
    # Configuration
    SAVE_PATH = "/home/dkhuttan/rgbt_project/rgbt_sim/RescueRandy/new_custom_plugin/thermal_saved"
    TOPIC = "/thermal/image"
    
    # Create saver and run
    saver = ThermalImageSaver(SAVE_PATH, TOPIC)
    saver.spin()