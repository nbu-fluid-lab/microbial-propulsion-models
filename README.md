# Funtrack Research Toolkit

This toolkit combines three computational components for reproducible research, scientific data exchange, and archival:

- [`experimental_data_processing`](experimental_data_processing/): Python tools and runnable examples for extracting metadata, trajectories, kinematics, and per-particle statistics from experiment videos.
- [`data_postprocessing`](data_postprocessing/): MATLAB functions for mean-squared displacement and species-specific effective-velocity analysis.
- [`numerical_simulation`](numerical_simulation/): C++ source code for the two-dimensional active-particle and obstacle-field simulation.

## Directory layout

```text
./
|-- README.md
|-- experimental_data_processing/
|   |-- README.md
|   |-- examples/
|   `-- package/
|-- data_postprocessing/
|   |-- README.md
|   |-- calculate_msd.m
|   |-- calculate_veff_chlamydomonas.m
|   `-- calculate_veff_escherichia_coli_paramecium.m
`-- numerical_simulation/
    |-- README.md
    |-- Makefile
    |-- main.cpp
    |-- cylinder.h
    `-- utils.h
```

All links and commands are relative to this README. The containing directory can therefore be renamed or relocated without changing any file in the toolkit.

The experimental examples emit JSON and CSV files, which are intentionally portable and can be loaded by SciDB or another scientific data platform using its normal ingestion tools. MATLAB postprocessing consumes trajectory and circle-analysis CSV files and writes derived CSV/XLSX results. The numerical simulation emits plain-text `.dat` files suitable for downstream conversion or ingestion.

## Recommended workflow

1. Install the supplied Python wheel and run an experiment-video example. See [`experimental_data_processing/README.md`](experimental_data_processing/README.md).
2. Run the required MSD or effective-velocity analysis on the generated CSV data. See [`data_postprocessing/README.md`](data_postprocessing/README.md).
3. Configure, build, and run the numerical model. See [`numerical_simulation/README.md`](numerical_simulation/README.md).
4. Preserve the original input video, generated outputs, parameter values, package version, and source revision together so that each result remains reproducible.

## Scope

This toolkit contains processing and simulation code. It does not contain experiment videos, generated results, credentials, or a deployment-specific SciDB loader.
