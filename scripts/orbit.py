

import math, time, subprocess, os

R = 7.0
z = 1.5
#steps = 72
#delay = 0.2

steps = 360
#delay = 0.05

#   os.makedirs("rgb_output", exist_ok=True)
#   os.makedirs("thermal_output", exist_ok=True)

for i in range(steps):
    temp = i%10
    if(temp<5):
        z+=0.1
    if(temp>=5):
        z-=0.1
    theta = 2 * math.pi * i / steps
    x = R * math.cos(theta)
    y = R * math.sin(theta)
    yaw = math.atan2(-y, -x)
    qz = math.sin(yaw / 2)
    qw = math.cos(yaw / 2)

    # Move rig
    cmd_pose = f"""gz service -s /world/thermal_world/set_pose \
        --reqtype gz.msgs.Pose --reptype gz.msgs.Boolean \
        --req 'name: "camera_rig" position {{x: {x}, y: {y}, z: {z}}} orientation {{x: 0, y: 0, z: {qz}, w: {qw}}}'"""
    subprocess.run(cmd_pose, shell=True)

    #time.sleep(delay)
