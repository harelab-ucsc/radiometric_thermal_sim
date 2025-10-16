#!/usr/bin/env python3
"""
Record camera_rig pose from Gazebo simulation using gz command-line tool
"""

import subprocess
import time
import csv
import re
import math
from datetime import datetime

class CameraPoseRecorder:
    def __init__(self, model_name="camera_rig", output_file="camera_poses.csv"):
        self.model_name = model_name
        self.output_file = output_file
        self.frame_count = 0
        self.start_time = time.time()
        
        print(f"Recording poses for model: {self.model_name}")
        print(f"Output file: {self.output_file}")
        print("Press Ctrl+C to stop recording\n")
        
        # Create CSV file with headers
        with open(self.output_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['frame', 'timestamp', 'x', 'y', 'z', 
                           'roll', 'pitch', 'yaw', 'qw', 'qx', 'qy', 'qz'])
    
    def get_model_pose(self):
        """Get pose using gz service call"""
        try:
            # Call gz service to get model pose
            result = subprocess.run(
                ['gz', 'model', '-m', self.model_name, '-p'],
                capture_output=True,
                text=True,
                timeout=1.0
            )
            
            if result.returncode != 0:
                return None
            
            # Parse the output
            output = result.stdout.strip()
            
            # The output format is:
            # - Pose [ XYZ (m) ] [ RPY (rad) ]:
            #   [x y z]
            #   [roll pitch yaw]
            
            # Extract XYZ line
            xyz_match = re.search(r'\[(\s*[-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s*\]', output)
            if not xyz_match:
                return None
            
            # Extract RPY line (second bracket pair)
            rpy_matches = re.findall(r'\[(\s*[-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s*\]', output)
            if len(rpy_matches) < 2:
                return None
            
            pose_match = True
            x = float(xyz_match.group(1))
            y = float(xyz_match.group(2))
            z = float(xyz_match.group(3))
            roll = float(rpy_matches[1][0])
            pitch = float(rpy_matches[1][1])
            yaw = float(rpy_matches[1][2])
            
            if pose_match:
                
                # Convert Euler to quaternion
                qw, qx, qy, qz = self.euler_to_quaternion(roll, pitch, yaw)
                
                return {
                    'x': x, 'y': y, 'z': z,
                    'roll': roll, 'pitch': pitch, 'yaw': yaw,
                    'qw': qw, 'qx': qx, 'qy': qy, 'qz': qz
                }
            
            return None
            
        except subprocess.TimeoutExpired:
            print("Warning: gz command timed out")
            return None
        except Exception as e:
            print(f"Error getting pose: {e}")
            return None
    
    def euler_to_quaternion(self, roll, pitch, yaw):
        """Convert Euler angles to quaternion"""
        cy = math.cos(yaw * 0.5)
        sy = math.sin(yaw * 0.5)
        cp = math.cos(pitch * 0.5)
        sp = math.sin(pitch * 0.5)
        cr = math.cos(roll * 0.5)
        sr = math.sin(roll * 0.5)
        
        qw = cr * cp * cy + sr * sp * sy
        qx = sr * cp * cy - cr * sp * sy
        qy = cr * sp * cy + sr * cp * sy
        qz = cr * cp * sy - sr * sp * cy
        
        return qw, qx, qy, qz
    
    def record_pose(self):
        """Get current pose and save to CSV"""
        pose = self.get_model_pose()
        
        if pose is None:
            return False
        
        self.frame_count += 1
        timestamp = time.time()
        
        # Print current pose
        if self.frame_count % 10 == 0:
            print(f"Frame {self.frame_count}: pos=({pose['x']:.3f}, {pose['y']:.3f}, {pose['z']:.3f}) "
                  f"orient=(roll={pose['roll']:.3f}, pitch={pose['pitch']:.3f}, yaw={pose['yaw']:.3f})")
        
        # Save to CSV
        with open(self.output_file, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                self.frame_count,
                timestamp,
                pose['x'],
                pose['y'],
                pose['z'],
                pose['roll'],
                pose['pitch'],
                pose['yaw'],
                pose['qw'],
                pose['qx'],
                pose['qy'],
                pose['qz']
            ])
        
        return True
    
    def run(self, rate_hz=10):
        """Keep recording at specified rate"""
        sleep_time = 1.0 / rate_hz
        
        try:
            print(f"Recording at {rate_hz} Hz...")
            while True:
                self.record_pose()
                time.sleep(sleep_time)
                
        except KeyboardInterrupt:
            elapsed = time.time() - self.start_time
            print(f"\n\nRecording stopped.")
            print(f"Total frames: {self.frame_count}")
            print(f"Duration: {elapsed:.2f} seconds")
            if elapsed > 0:
                print(f"Average FPS: {self.frame_count/elapsed:.2f}")
            print(f"Data saved to {self.output_file}")

if __name__ == "__main__":
    # You can change the recording rate here (Hz)
    recorder = CameraPoseRecorder(model_name="camera_rig", output_file="camera_poses.csv")
    recorder.run(rate_hz=10)  # Record 10 times per second