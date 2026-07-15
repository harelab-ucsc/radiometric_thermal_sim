# radiometric_thermal_sim

A Gazebo (gz-sim / Gazebo Harmonic) simulation with a custom sensor plugin that turns Gazebo's non-radiometric thermal camera into a proper **radiometric** thermal camera — meaning every pixel comes out as an actual temperature in Kelvin, not just a normalized grayscale image for looking at.

The scene is a small search-and-rescue setup: a "Rescue Randy" mannequin, a phone, a backpack, and a couple of objects with different temperatures, sitting on Gazebo's Baylands terrain. A camera rig with a paired RGB + thermal camera looks at the scene from a fixed point, and the plugin subscribes to the thermal topic, decodes the raw sensor data into real temperatures, and writes it out as `.raw` float32 files you can load in numpy.

![Simulation world](/media/sim_world.png)

## Why this exists

Gazebo's built-in thermal camera is great for visualization, but out of the box it just gives you an 8- or 16-bit grayscale image — there's no straightforward way to get calibrated temperature values out of it for downstream use (dataset generation, model training, etc). This project adds that layer: a custom `gz-sim` system plugin (`RadiometricThermalSensor`) that listens to the thermal topic and converts the raw pixel data back into Kelvin, plus a couple of Python scripts for post-processing and inspecting the output.

Getting the conversion actually correct took a while — Gazebo's thermal camera encoding isn't documented all that clearly, and it's easy to get plausible-looking-but-wrong numbers if you assume the wrong formula. See [How the temperature conversion actually works](#how-the-temperature-conversion-actually-works) below if you're touching this code.

## What's in here

```
worlds/thermalRandy.sdf              the world file — this is what you actually run
plugins/radiometric_thermal_sensor/  the custom C++ sensor plugin (gz-sim system)
models/rescue_randy/                 the mannequin model (materials, mesh, thumbnails)
media/skybox/                        the sky cubemap texture used by the world
scripts/                             Python helpers for moving the camera and processing output
output/                              gitignored — RGB/thermal PNGs and radiometric .raw files land here
```

The phone, backpack, and terrain models aren't stored in this repo — they're pulled from Gazebo Fuel automatically the first time you run the world (see [Requirements](#requirements)).

## Requirements

- **Gazebo Harmonic** (`gz-sim9`) with the dev packages for `gz-cmake3`, `gz-common6`, `gz-msgs11`, and `gz-transport14` — this is what the plugin links against. If `gz sim --version` reports something other than a 9.x release, the plugin likely won't build against your install without adjusting `plugins/radiometric_thermal_sensor/CMakeLists.txt`.
- A working OpenGL setup for `ogre2` rendering. If you're on a machine with an NVIDIA GPU and see `egl: failed to create dri2 screen` warnings on startup, that's usually harmless, but if the thermal camera output looks wrong (e.g. saturated/uniform after the first frame), see [Known issues](#known-issues).
- Internet access the first time you run the world, so Gazebo can download Rescue Randy, the phone, the backpack, and the Baylands terrain from Fuel.
- Python 3 with `numpy`, `pillow`, and `matplotlib` for the post-processing scripts. `scripts/orbit_save.py` additionally needs `imageio`.

## Getting started

1. **Clone and run the setup script.** The world file needs real absolute filesystem paths (for the plugin location and where output gets saved), so there's a one-time substitution step:

   ```bash
   git clone <this repo>
   cd radiometric_thermal_sim
   ./setup.sh
   ```

   This fills in your actual clone path in `worlds/thermalRandy.sdf` and creates the `output/` folder. Safe to re-run; it's a no-op if you run it again from the same location.

2. **Build the plugin:**

   ```bash
   mkdir -p plugins/radiometric_thermal_sensor/build
   cd plugins/radiometric_thermal_sensor/build
   cmake ..
   cmake --build .
   cd ../../..
   ```

3. **Run it:**

   ```bash
   gz sim worlds/thermalRandy.sdf
   ```

   The sim starts paused — hit play in the World Control widget. You should see two image display panels (RGB and thermal) pop up alongside the 3D view. The first time you run this, Gazebo will download the Fuel models, which can take a minute depending on your connection.

   Console output from the plugin will tell you it's subscribed, and every frame it saves will print the frame's raw value range and decoded temperature range in Kelvin.

## Using it

### Where output goes

Three things get written to `output/` while the sim runs:
- `output/rgb/` — RGB camera frames as PNG (Gazebo's built-in camera save)
- `output/thermal/` — raw thermal camera frames as 16-bit PNG (Gazebo's built-in camera save, not yet converted to Kelvin)
- `output/radiometric/` — `thermal_<N>.raw` files from the custom plugin: an 8-byte header (width, height as `unsigned int`) followed by a `width × height` array of `float32` temperatures in Kelvin

### Moving the camera

The camera rig is a static model, so if you want more than one viewpoint you move it with `gz service` calls. Two scripts do this for you:

- `scripts/orbit.py` — circles the rig around the scene at a fixed radius, bobbing the height up and down slightly as it goes.
- `scripts/aeriel_path.py` — flies a back-and-forth serpentine sweep over the scene from higher up, useful for aerial-style coverage.

Both are plain scripts, not CLIs — open them up and adjust the radius/height/step count at the top before running, then just `python3 scripts/orbit.py` while the sim is running.

If you want to log the camera's actual pose alongside the images you're capturing (e.g. to pair with a SLAM/reconstruction pipeline), run `scripts/get_pose.py` in a separate terminal while the sim is running — it polls `gz model -m camera_rig -p` at 10 Hz and appends rows to `camera_poses.csv` (position, RPY, and the equivalent quaternion).

### Working with the saved data

To check a `.raw` file's temperature range and eyeball it:

```bash
python3 scripts/read.py output/radiometric/thermal_0.raw
```

This prints min/max/mean temperature and offers to pop up a matplotlib heatmap with a Kelvin colorbar.

If you'd rather convert the PNGs Gazebo already saved in `output/thermal/` instead of relying on the live topic subscription (useful if you only care about the frames you explicitly saved, or want to reprocess with a different resolution later):

```bash
python3 scripts/png_to_radiometric.py output/thermal/<file>.png output/radiometric/<file>.raw
# or batch a whole directory:
python3 scripts/png_to_radiometric.py output/thermal/ output/radiometric/
```

This only works on 16-bit (`L16`) thermal PNGs — the `<image><format>` in the world file's `thermal_camera` sensor needs to stay `L16`, not `L8`, or this script will refuse the input rather than silently produce garbage.

## How the temperature conversion actually works

This tripped me up enough that it's worth writing down properly. Gazebo's thermal camera does **not** normalize temperatures to fill the pixel format's full range per-frame. It's a fixed, linear unit conversion:

```
raw_pixel_value = clamp(temperature_kelvin, min_temp, max_temp) / resolution
```

or equivalently, to go from a saved raw value back to a temperature:

```
temperature_kelvin = raw_pixel_value * resolution
```

`resolution` is Kelvin-per-count — Gazebo's own default is `0.01` for a 16-bit camera. `min_temp` and `max_temp` only **clamp** the input temperature before that division; they are not an offset baked into the conversion (a temperature below `min_temp` is reported as if it were exactly `min_temp`, not as zero).

The practical consequence: the `resolution` value configured on the sensor's `ThermalSensor` calibration plugin (the encoder, in `worlds/thermalRandy.sdf`) and the `resolution` used by `RadiometricThermalSensor` and `scripts/png_to_radiometric.py` (the decoders) **must be identical**, or the decoded temperatures will be wrong — either collapsed near `min_temp` (resolution too coarse relative to what was actually used to encode) or fully saturated at `max_temp` (resolution too fine). Both of these failure modes are easy to hit and don't throw any error; they just quietly produce wrong numbers, which is exactly what happened during development here.

## Known issues

- **Fuel dependency**: the world won't fully load without internet access on first run (or if the Fuel cache at `~/.gz/fuel/` gets cleared).
- **`scripts/orbit_save.py`** assumes 8-bit pixel data when decoding a `gz topic -e` capture. It works fine for the RGB topic but will misdecode the 16-bit thermal topic — it's a quick one-off capture helper, not something to rely on for thermal data (use the radiometric plugin output or `png_to_radiometric.py` instead).
- **GPU rendering**: on a machine where the offscreen render context Gazebo uses for sensors doesn't get properly hardware-accelerated (some multi-GPU / remote-desktop setups), the thermal camera can render fine for the first frame or two and then saturate to a single uniform value on every frame after. If you see that, check `nvidia-smi` and confirm `/usr/share/glvnd/egl_vendor.d/` has your GPU vendor's EGL ICD registered.
- The `skybox.exr` cubemap is ~92MB — comfortably under GitHub's hard file size limit, but worth knowing if you're cloning on a slow connection.

## License

Apache 2.0 — see [LICENSE](LICENSE).
