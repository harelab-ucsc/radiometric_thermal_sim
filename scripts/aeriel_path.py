

import math, time, subprocess, os

R = 7.0
z = 15.0
#steps = 72

steps = 200
counter = 1
x =0
y = -20
direction = 1
delay = 0.1
theta = math.radians(45)
wy = math.sin(theta / 2.0)
ww = math.cos(theta / 2.0)

#   os.makedirs("rgb_output", exist_ok=True)
#   os.makedirs("thermal_output", exist_ok=True)

for i in range(steps):
    if(counter%10 == 0):
        direction *= -1
        y += 1.5
    x += direction    
    counter +=1
    # Move rig
    cmd_pose = f"""gz service -s /world/thermal_world/set_pose \
        --reqtype gz.msgs.Pose --reptype gz.msgs.Boolean \
        --req 'name: "camera_rig" position {{x: {x}, y: {y}, z: {z}}} orientation {{x: 0, y: {wy}, z: 0, w: {ww}}}'"""
    subprocess.run(cmd_pose, shell=True)

    time.sleep(delay)

    
