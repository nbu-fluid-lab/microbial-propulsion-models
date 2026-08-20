# Data Postprocessing

This component provides MATLAB functions for calculating mean-squared displacement (MSD) and effective velocity (`Veff`) from trajectory-analysis CSV files.

## Requirements

- MATLAB R2020b or newer
- MATLAB access to the directory containing these `.m` files
- CSV inputs produced by the trajectory and circle-analysis workflows

The functions use MATLAB base functionality and do not require a statistics toolbox.

## Setup

Add this directory to the MATLAB search path once per session:

```matlab
addpath('/absolute/path/to/data_postprocessing');
```

Each function accepts the data directory explicitly. The source files can therefore remain separate from experiment data and generated results.

## Mean-squared displacement

```matlab
calculate_msd('/absolute/path/to/trajectory_csv_files');
```

`calculate_msd` reads every CSV file directly inside the supplied directory. Required columns are resolved by name when possible:

| Quantity | Accepted column names | Positional fallback |
| --- | --- | --- |
| Video identifier | `video_id`, `video`, `movie_id` | Input filename |
| Frame | `frame`, `frames` | Column 2 |
| Time | `time`, `t` | Column 3 |
| Track identifier | `idx`, `id`, `track_id`, `particle_id` | Column 4 |
| X coordinate | `x`, `x_px`, `pos_x`, `x_position` | Column 5 |
| Y coordinate | `y`, `y_px`, `pos_y`, `y_position` | Column 6 |

Important default parameters are defined near the top of `calculate_msd.m`:

| Parameter | Default | Meaning |
| --- | --- | --- |
| `minAverageSpeed` | `200` | Minimum trajectory-average speed |
| `durationRatio` | `0.90` | Minimum duration relative to the longest retained trajectory |
| `minSelectedTracks` | `3` | Minimum number of long trajectories selected when available |
| `minPoints` | `10` | Minimum points in a candidate trajectory |
| `minTracksPerLag` | `1` | Minimum contributing tracks at each lag |

Results are written to `MSD_raw_speedFiltered_longTime_output/` inside the data directory:

- `ALL_raw_speedFiltered_track_summary.csv`
- `ALL_raw_speedFiltered_longTime_average_MSD.csv`

Coordinates, time, and speed retain the units used in the input CSV files. Consequently, MSD uses the square of the input coordinate unit.

## Effective velocity for Chlamydomonas

```matlab
calculate_veff_chlamydomonas('/absolute/path/to/analysis_root');
```

The function searches the supplied root and each immediate child directory. It pairs files when:

- one filename contains `circle`;
- the other filename contains `kinematics`; and
- both filenames start with the same numeric prefix.

The circle table must contain at least 15 columns. The function reads video ID, track ID, entry/exit frames, entry/exit times, inside duration, and inside distance from columns 1, 2, 3, 4, 5, 6, 13, and 15. The kinematics table must contain at least 9 columns; video ID, track ID, frame, time, angle, and speed are read from columns 1, 2, 3, 4, 7, and 9.

Key defaults are a `0.25 s` frame interval, a 45-degree straight-motion threshold, the 50th speed percentile, and a maximum translational speed of `150`.

## Effective velocity for Escherichia coli and Paramecium

```matlab
calculate_veff_escherichia_coli_paramecium('/absolute/path/to/analysis_root');
```

File discovery and column positions follow the same rules as the Chlamydomonas function. This variant also reads entry and exit coordinates from circle-table columns 7 through 10. Its defaults use a 20-degree straight-motion threshold, the 60th speed percentile, and a maximum translational speed of `30`.

Both effective-velocity functions calculate:

```text
Veff = (inside distance / inside duration) / translational speed
```

## Effective-velocity outputs

Each processed data directory may receive:

- `Veff_All_Rows_In_This_Folder.csv`
- `Veff_Folder_Summary.csv`
- `Veff_Skipped_Rows.csv`

The supplied root directory receives combined outputs:

- `All_Folders_All_Veff.csv`
- `All_Folders_Veff_Summary.csv`
- `All_Folders_Skipped_Rows.csv`
- `All_Folders_Veff_Result.xlsx`

Existing files with these names may be replaced. Preserve a copy of previous results before rerunning an analysis.

## Scientific validation

The default thresholds are species-specific analysis choices, not universal constants. Confirm coordinate units, time units, file-column order, speed limits, and trajectory-selection thresholds for each experiment before interpreting or publishing the results.
