# Experimental Data Processing

This package processes microscopy and particle-motion videos with `funtrack-core`. It includes an installable wheel and three examples that require only a local video path.

## Requirements

- Python 3.12 or newer
- A video format supported by the OpenCV build on the target machine
- Enough disk space for CSV outputs

## Install the supplied package

From this directory, create an isolated environment and install the wheel:

```bash
python3.12 -m venv .venv
source .venv/bin/activate
python -m pip install package/funtrack_core-1.1.100-py3-none-any.whl
```

The wheel contains `funtrack-core` itself. During installation, pip also resolves its declared runtime dependencies. Configure the appropriate Python package index before installation when working in an offline or private network.

Confirm the installation:

```bash
python -c "import funtrack; print(funtrack.__file__)"
```

## Examples

Each command accepts exactly one positional argument: the input video path. Results are created under `funtrack_results/<video-stem>/` in the current working directory.

### 1. Inspect video metadata

```bash
python examples/inspect_video.py /absolute/path/to/experiment.mp4
```

Output: `video_info.json`, including frame size, frame rate, frame count, duration, and file size.

### 2. Extract trajectories

```bash
python examples/track_video.py /absolute/path/to/experiment.mp4
```

Outputs:

- `tracks.csv`: one row per tracked position, with `frame`, `time`, `idx`, `x`, and `y` columns
- `tracking_summary.json`: tracker parameters and a summary for every trajectory

Coordinates in `tracks.csv` are pixels, and time is seconds.

### 3. Run the complete single-video workflow

```bash
python examples/analyze_video.py /absolute/path/to/experiment.mp4
```

Outputs:

- `video_info.json`: source-video metadata
- `tracks.csv`: raw trajectories in pixels
- `kinematics.csv`: velocity, direction, and angular velocity derived from each trajectory
- `particle_statistics.csv`: one summary row per tracked particle
- `tracking_summary.json`: tracker configuration and trajectory summary
- `manifest.json`: input, output, package, and unit information for reproducibility

The complete example deliberately preserves pixel-based units because a video path alone does not provide a physical calibration. Use the experiment's micrometers-per-pixel calibration before interpreting distances or speeds as physical quantities.

## Automatic parameters and scientific validation

The tracking examples estimate detection area and association distance from sampled video frames before processing the full video. This makes the one-argument workflow practical, but it does not remove the need for scientific validation. Review the trajectory overlay or a representative subset of `tracks.csv` before using results in an analysis or publication.

For difficult recordings, adapt the tracker parameters in `examples/_common.py`, especially `min_area`, `max_area`, `max_distance`, background subtraction, and preprocessing options.

## SciDB ingestion

The generated CSV files use a header row and comma-separated values. A typical schema mapping is:

| File | Suggested dimensions | Suggested attributes |
| --- | --- | --- |
| `tracks.csv` | `idx`, `frame` | `time`, `x`, `y` |
| `kinematics.csv` | `idx`, `frame` | `time`, `x_px`, `y_px`, `speed_px_per_s`, `angle_rad`, `angular_velocity_rad_per_s` |
| `particle_statistics.csv` | `idx` | counts, time range, displacement, distance, and speed statistics |

Choose SciDB dimensions and chunk sizes according to the dataset volume and the queries that will be run. The exact loader command is deployment-specific and is therefore not included in this toolkit.
