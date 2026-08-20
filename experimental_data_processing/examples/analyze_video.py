#!/usr/bin/env python3
"""Run metadata, tracking, kinematics, and statistics for one video."""

from __future__ import annotations

import argparse
from importlib.metadata import PackageNotFoundError, version
from typing import Any

import numpy as np
import pandas as pd
from _common import require_video, result_directory, track_video, write_json

from funtrack import (
    calculate_angle,
    calculate_angular_velocity,
    calculate_velocity,
    extract_video_info,
)

KINEMATICS_COLUMNS = [
    "idx",
    "frame",
    "time",
    "x_px",
    "y_px",
    "vx_px_per_s",
    "vy_px_per_s",
    "speed_px_per_s",
    "angle_rad",
    "cumulative_angle_rad",
    "angular_velocity_rad_per_s",
]

STATISTICS_COLUMNS = [
    "idx",
    "frame_count",
    "start_time_s",
    "end_time_s",
    "duration_s",
    "start_x_px",
    "start_y_px",
    "end_x_px",
    "end_y_px",
    "net_displacement_px",
    "total_distance_px",
    "mean_speed_px_per_s",
    "max_speed_px_per_s",
]


def build_kinematics(tracks: pd.DataFrame) -> pd.DataFrame:
    if tracks.empty:
        return pd.DataFrame(columns=KINEMATICS_COLUMNS)

    data = calculate_velocity(tracks, scale_um_per_pixel=1.0)
    data = calculate_angle(data)
    data = calculate_angular_velocity(data)
    data["cumulative_angle"] = data["dangle"].fillna(0).groupby(data["idx"]).cumsum()

    data = data.rename(
        columns={
            "x": "x_px",
            "y": "y_px",
            "vx": "vx_px_per_s",
            "vy": "vy_px_per_s",
            "speed": "speed_px_per_s",
            "angle": "angle_rad",
            "cumulative_angle": "cumulative_angle_rad",
            "angular_velocity": "angular_velocity_rad_per_s",
        }
    )
    return data[KINEMATICS_COLUMNS].reset_index(drop=True)


def build_statistics(kinematics: pd.DataFrame) -> pd.DataFrame:
    if kinematics.empty:
        return pd.DataFrame(columns=STATISTICS_COLUMNS)

    rows: list[dict[str, Any]] = []
    for track_id, group in kinematics.groupby("idx", sort=True):
        group = group.sort_values("frame")
        dx = group["x_px"].diff().fillna(0.0)
        dy = group["y_px"].diff().fillna(0.0)
        start = group.iloc[0]
        end = group.iloc[-1]
        rows.append(
            {
                "idx": track_id,
                "frame_count": len(group),
                "start_time_s": start["time"],
                "end_time_s": end["time"],
                "duration_s": end["time"] - start["time"],
                "start_x_px": start["x_px"],
                "start_y_px": start["y_px"],
                "end_x_px": end["x_px"],
                "end_y_px": end["y_px"],
                "net_displacement_px": np.hypot(
                    end["x_px"] - start["x_px"], end["y_px"] - start["y_px"]
                ),
                "total_distance_px": np.hypot(dx, dy).sum(),
                "mean_speed_px_per_s": group["speed_px_per_s"].mean(),
                "max_speed_px_per_s": group["speed_px_per_s"].max(),
            }
        )

    return pd.DataFrame(rows, columns=STATISTICS_COLUMNS)


def package_version() -> str:
    try:
        return version("funtrack-core")
    except PackageNotFoundError:
        return "source-checkout"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("video", help="Path to the experiment video")
    args = parser.parse_args()

    video = require_video(args.video)
    output = result_directory(video)
    info = extract_video_info(str(video))
    if "error" in info:
        raise SystemExit(f"Could not read the video: {info['error']}")

    tracks, summary, parameters = track_video(video)
    kinematics = build_kinematics(tracks)
    statistics = build_statistics(kinematics)

    outputs = {
        "video_info": output / "video_info.json",
        "tracks": output / "tracks.csv",
        "kinematics": output / "kinematics.csv",
        "particle_statistics": output / "particle_statistics.csv",
        "tracking_summary": output / "tracking_summary.json",
        "manifest": output / "manifest.json",
    }
    write_json(outputs["video_info"], info)
    tracks.to_csv(outputs["tracks"], index=False)
    kinematics.to_csv(outputs["kinematics"], index=False)
    statistics.to_csv(outputs["particle_statistics"], index=False)
    write_json(
        outputs["tracking_summary"],
        {"estimated_parameters": parameters, **summary},
    )
    write_json(
        outputs["manifest"],
        {
            "input_video": str(video),
            "funtrack_core_version": package_version(),
            "coordinate_unit": "pixel",
            "time_unit": "second",
            "speed_unit": "pixel/second",
            "angle_unit": "radian",
            "physical_scale_um_per_pixel": None,
            "outputs": {name: str(path) for name, path in outputs.items()},
            "track_count": summary["total_tracks"],
            "trajectory_row_count": len(tracks),
        },
    )

    print(f"Tracks: {summary['total_tracks']}")
    print(f"Trajectory rows: {len(tracks)}")
    print(f"Results written to {output}")


if __name__ == "__main__":
    main()
