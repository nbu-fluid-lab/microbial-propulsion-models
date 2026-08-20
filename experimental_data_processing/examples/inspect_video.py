#!/usr/bin/env python3
"""Extract metadata from one video and write it as JSON."""

from __future__ import annotations

import argparse

from _common import require_video, result_directory, write_json

from funtrack import extract_video_info


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("video", help="Path to the experiment video")
    args = parser.parse_args()

    video = require_video(args.video)
    output = result_directory(video) / "video_info.json"
    info = extract_video_info(str(video))
    if "error" in info:
        raise SystemExit(f"Could not read the video: {info['error']}")

    write_json(output, info)
    print(f"Video metadata written to {output}")


if __name__ == "__main__":
    main()
