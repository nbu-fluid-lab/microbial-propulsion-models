"""Shared helpers for the single-video examples."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import pandas as pd

from funtrack import BacteriaTracker, estimate_detection_params

TRACK_COLUMNS = ["frame", "time", "idx", "x", "y"]


def require_video(value: str) -> Path:
    """Return an absolute video path or stop with a useful error."""
    video = Path(value).expanduser().resolve()
    if not video.is_file():
        raise SystemExit(f"Video file does not exist: {video}")
    return video


def result_directory(video: Path) -> Path:
    """Create the conventional result directory for a video."""
    output = Path.cwd() / "funtrack_results" / video.stem
    output.mkdir(parents=True, exist_ok=True)
    return output


def _json_default(value: Any) -> Any:
    if isinstance(value, Path):
        return str(value)
    item = getattr(value, "item", None)
    if callable(item):
        return item()
    raise TypeError(f"Object of type {type(value).__name__} is not JSON serializable")


def write_json(path: Path, value: Any) -> None:
    path.write_text(
        json.dumps(value, indent=2, ensure_ascii=True, default=_json_default) + "\n",
        encoding="utf-8",
    )


def track_video(video: Path) -> tuple[pd.DataFrame, dict[str, Any], dict[str, Any]]:
    """Estimate practical defaults, track the video, and normalize an empty result."""
    parameters = estimate_detection_params(str(video))
    tracker = BacteriaTracker(video_path=str(video), **parameters)
    tracker.process_video()

    tracks = tracker.tracks_dataframe()
    if tracks.empty:
        tracks = pd.DataFrame(columns=TRACK_COLUMNS)
    else:
        tracks = tracks[TRACK_COLUMNS]

    return tracks, tracker.get_tracks_summary(), parameters
