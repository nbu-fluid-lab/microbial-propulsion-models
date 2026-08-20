# Research Data

This directory contains the derived research dataset associated with the analysis and simulation code in this repository. The original `raw_data.tar.gz` archive is split into Git-friendly parts so that no tracked file exceeds GitHub's per-file limits or 50 MB warning threshold.

## Archive metadata

| Property | Value |
| --- | --- |
| Original filename | `raw_data.tar.gz` |
| Original size | `161,227,570` bytes |
| Original SHA-256 | `a5549071d89c767a63e73df303203bda1c7e9ca320ab2a4cf0ba71ad684d1d22` |
| Number of parts | `4` |
| Maximum part size | `48,000,000` bytes |

The archive contains 220 CSV files with trajectory and normalized analysis data for *Chlamydomonas*, *Escherichia coli*, and *Paramecium*.

## Reassemble the archive

From the repository root:

```bash
./data/reassemble_raw_data.sh
```

The script verifies every part, reconstructs `data/raw_data.tar.gz`, verifies the complete archive SHA-256, and checks the tar/gzip structure. It never overwrites an existing invalid archive.

To write the reconstructed archive elsewhere:

```bash
./data/reassemble_raw_data.sh /absolute/path/to/raw_data.tar.gz
```

## Extract the dataset

```bash
cd data
tar -xzf raw_data.tar.gz
```

The resulting `data/raw_data/` directory is intentionally ignored by Git.

## Extracted layout

```text
raw_data/
|-- trajectory/
|   |-- Chlamydomonas/
|   |-- Escherichia coli/
|   `-- Paramecium/
|-- L_normalized/
|-- V_eff_normalized/
`-- angle_change_deg/
```

The trajectory directories are grouped by species and experimental condition. The normalized directories contain species-level derived tables used by the paper analysis.

## Integrity files

- `PARTS.sha256` records the SHA-256 checksum of every tracked part.
- `RAW_DATA.sha256` records the SHA-256 checksum of the reconstructed archive.

Do not edit individual part files. Any byte-level modification prevents successful verification and reconstruction.
