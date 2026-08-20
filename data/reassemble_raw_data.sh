#!/usr/bin/env bash

set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
parts_dir="$script_dir/parts"
parts_manifest="$script_dir/PARTS.sha256"
archive_manifest="$script_dir/RAW_DATA.sha256"

usage() {
    cat <<'EOF'
Usage: reassemble_raw_data.sh [OUTPUT_ARCHIVE]

Verify all tracked parts and reconstruct raw_data.tar.gz. When OUTPUT_ARCHIVE
is omitted, the archive is written next to this script. A relative output path
is resolved from the current working directory.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if (( $# > 1 )); then
    usage >&2
    exit 2
fi

requested_output="${1:-}"

sha256_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        echo "A SHA-256 tool (sha256sum or shasum) is required." >&2
        return 1
    fi
}

if [[ ! -f "$parts_manifest" || ! -f "$archive_manifest" ]]; then
    echo "Checksum manifests are missing from $script_dir" >&2
    exit 1
fi

expected_part_count="$(awk 'NF >= 2 {count++} END {print count + 0}' "$parts_manifest")"
set -- "$parts_dir"/raw_data.tar.gz.part-*
if [[ ! -e "$1" ]]; then
    echo "No archive parts were found in $parts_dir" >&2
    exit 1
fi

actual_part_count=$#
if [[ "$actual_part_count" -ne "$expected_part_count" ]]; then
    echo "Expected $expected_part_count parts, found $actual_part_count." >&2
    exit 1
fi

while read -r expected_hash relative_path; do
    [[ -n "$expected_hash" && -n "$relative_path" ]] || continue
    part_path="$script_dir/$relative_path"
    if [[ ! -f "$part_path" ]]; then
        echo "Missing archive part: $part_path" >&2
        exit 1
    fi
    actual_hash="$(sha256_file "$part_path")"
    if [[ "$actual_hash" != "$expected_hash" ]]; then
        echo "Checksum mismatch: $part_path" >&2
        exit 1
    fi
    echo "Verified: $relative_path"
done < "$parts_manifest"

if [[ -n "$requested_output" ]]; then
    output_input="$requested_output"
    if [[ "$output_input" = /* ]]; then
        output_candidate="$output_input"
    else
        output_candidate="$PWD/$output_input"
    fi
else
    output_candidate="$script_dir/raw_data.tar.gz"
fi

output_dir="$(dirname -- "$output_candidate")"
if [[ ! -d "$output_dir" ]]; then
    echo "Output directory does not exist: $output_dir" >&2
    exit 1
fi
output_dir="$(CDPATH= cd -- "$output_dir" && pwd)"
output_path="$output_dir/$(basename -- "$output_candidate")"
expected_archive_hash="$(awk 'NF >= 1 {print $1; exit}' "$archive_manifest")"

if [[ -f "$output_path" ]]; then
    existing_hash="$(sha256_file "$output_path")"
    if [[ "$existing_hash" == "$expected_archive_hash" ]]; then
        echo "Archive already exists and is valid: $output_path"
        exit 0
    fi
    echo "Refusing to overwrite an invalid existing archive: $output_path" >&2
    exit 1
elif [[ -e "$output_path" ]]; then
    echo "Output path exists and is not a regular file: $output_path" >&2
    exit 1
fi

temporary_archive="$(mktemp "$output_dir/.raw_data.tar.gz.tmp.XXXXXX")"
trap 'rm -f -- "$temporary_archive"' EXIT

cat "$parts_dir"/raw_data.tar.gz.part-* > "$temporary_archive"
actual_archive_hash="$(sha256_file "$temporary_archive")"
if [[ "$actual_archive_hash" != "$expected_archive_hash" ]]; then
    echo "Reassembled archive checksum mismatch." >&2
    exit 1
fi

tar -tzf "$temporary_archive" >/dev/null
mv -- "$temporary_archive" "$output_path"
chmod 0644 "$output_path"
trap - EXIT

echo "Archive created and verified: $output_path"
