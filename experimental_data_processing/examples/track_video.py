#!/usr/bin/env python3
"""Track particles in one video and write trajectory data."""

from __future__ import annotations

import argparse

from _common import require_video, result_directory, track_video, write_json


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("video", help="Path to the experiment video")
    args = parser.parse_args()

    video = require_video(args.video)
    output = result_directory(video)
    tracks, summary, parameters = track_video(video)

    tracks_path = output / "tracks.csv"
    summary_path = output / "tracking_summary.json"
    tracks.to_csv(tracks_path, index=False)
    write_json(
        summary_path,
        {
            "video_path": str(video),
            "coordinate_unit": "pixel",
            "time_unit": "second",
            "estimated_parameters": parameters,
            **summary,
        },
    )

    print(f"Tracks: {summary['total_tracks']}")
    print(f"Trajectory rows: {len(tracks)}")
    print(f"Results written to {output}")


if __name__ == "__main__":
    main()
