# A geometry-driven transport crossover emerges across microbial propulsion modes

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.22043249.svg)](https://doi.org/10.5281/zenodo.22043249)

Code and processed data accompanying the manuscript **"A geometry-driven
transport crossover emerges across microbial propulsion modes."**

This repository combines experimental trajectory processing, quantitative
postprocessing, and a two-dimensional hydrodynamic simulation used to examine
how obstacle geometry affects transport across three microorganisms with
different propulsion modes:

- *Chlamydomonas* (a biflagellate green alga)
- *Escherichia coli* (a flagellated bacterium)
- *Paramecium* (a ciliate)

## Repository contents

| Path | Contents | Main requirements |
| --- | --- | --- |
| [`data/`](data/) | Verified split archive containing 211 trajectory CSV files and 9 species-level derived tables | Bash, `tar`, and a SHA-256 utility |
| [`experimental_data_processing/`](experimental_data_processing/) | A packaged video-tracking workflow and examples for metadata, trajectories, kinematics, and particle statistics | Python 3.12+ |
| [`data_postprocessing/`](data_postprocessing/) | MATLAB functions for mean-squared displacement (MSD) and effective velocity (`Veff`) | MATLAB R2020b+ |
| [`numerical_simulation/`](numerical_simulation/) | D2Q9 lattice-Boltzmann and immersed-boundary simulation of an active elliptical particle in a circular-obstacle field | C++17 and `make` |

Each component has its own README with input formats, parameters, outputs, and
scientific validation notes.

## Quick start

All commands below are run from the repository root.

### 1. Reconstruct the processed dataset

The data archive is split into four parts to keep each tracked file below
GitHub's per-file limits. The reconstruction script verifies both the parts and
the complete archive before keeping the result.

```bash
./data/reassemble_raw_data.sh
tar -xzf data/raw_data.tar.gz -C data
```

The extracted files are written to `data/raw_data/`, which is ignored by Git.
See [`data/README.md`](data/README.md) for checksums and archive details.

### 2. Recalculate an MSD curve

Start MATLAB in the repository root and run, for example:

```matlab
addpath('data_postprocessing');
calculate_msd(fullfile('data', 'raw_data', 'trajectory', ...
    'Chlamydomonas', 'n_0'));
```

Results are written inside the selected input directory under
`MSD_raw_speedFiltered_longTime_output/`. The default trajectory filters and
output schemas are documented in
[`data_postprocessing/README.md`](data_postprocessing/README.md).

### 3. Build the numerical model

```bash
make -C numerical_simulation
```

This creates `numerical_simulation/build/funtrack_simulation`. Before starting
a production run, review the compile-time model parameters in `cylinder.h` and
replace the very large default loop bound in `main.cpp` with an appropriate
stopping condition. Run each parameter set in a separate working directory;
the simulation writes its `.dat` outputs relative to the current directory.
See [`numerical_simulation/README.md`](numerical_simulation/README.md) for the
parameter table, run command, output layout, and random-seed caveat.

### 4. Process a new experiment video

Original microscopy videos are not included. To process a local video with the
packaged tracker:

```bash
cd experimental_data_processing
python3.12 -m venv .venv
source .venv/bin/activate
python -m pip install package/funtrack_core-1.1.100-py3-none-any.whl
python examples/analyze_video.py /absolute/path/to/experiment.mp4
```

The example writes portable CSV and JSON results to
`funtrack_results/<video-stem>/`. Its automatically estimated tracking
parameters must be validated against representative frames before the results
are used scientifically. See
[`experimental_data_processing/README.md`](experimental_data_processing/README.md)
for the individual examples and output columns.

## Data organization

After extraction, the dataset has the following structure:

```text
data/raw_data/
|-- trajectory/
|   |-- Chlamydomonas/
|   |-- Escherichia coli/
|   `-- Paramecium/
|-- L_normalized/
|-- V_eff_normalized/
`-- angle_change_deg/
```

The trajectory data are grouped by organism and by geometry condition:
`n_0`, `n_3`, `n_5`, `n_7`, `n_9`, and `n_11`. The meaning of each condition
and the normalization conventions should be interpreted using the manuscript's
Methods section.

| Dataset | Files | Description |
| --- | ---: | --- |
| `trajectory/` | 211 | Per-video kinematics tables grouped by organism and geometry condition |
| `L_normalized/` | 3 | Species-level normalized path-length results |
| `V_eff_normalized/` | 3 | Species-level normalized effective-velocity results |
| `angle_change_deg/` | 3 | Species-level angular-change results in degrees |

## Reproducibility scope

The repository supports several workflows, but it is not a single-command
reproduction package for every manuscript figure:

- MSD can be recalculated directly from the bundled trajectory tables.
- The `Veff` functions require paired circle-analysis and kinematics CSV files.
  The bundled archive contains the derived normalized `Veff` tables, but not
  the source circle-analysis files.
- Original microscopy videos are not distributed, so the experimental tracking
  stage requires user-provided recordings.
- Figure-generation scripts are not included.
- Exact simulation reruns require recording fixed random seeds, source revision,
  compiler version, parameter constants, and stopping condition.

Keep input data, generated outputs, parameter choices, software versions, and
the Git commit identifier together when archiving a result.

## Author information

Affiliations:

1. Laboratory of Impact and Safety Engineering (Ningbo University), Ministry
   of Education, Ningbo 315201, China
2. State Key Laboratory of Fluid Power Transmission and Control, Zhejiang
   University, Hangzhou 310027, China
3. Department of Mechanical Engineering, National University of Singapore,
   Singapore 117575, Singapore
4. Mekong University (MKU), Vinh Long Province, Vietnam

| Author | Affiliation(s) | CRediT contributions |
| --- | --- | --- |
| Chen Liu | 1 | Writing - original draft; Software; Data curation |
| Xiangyu Zhang | 1 | Writing - original draft; Visualization; Data curation; Investigation |
| Zhenyu Ouyang | 1 | Writing - review and editing; Supervision; Project administration |
| Jianzhong Lin | 1, 2 | Writing - review and editing |
| Nhan Phan-Thien | 3, 4 | Writing - review and editing |

Correspondence: Zhenyu Ouyang, `ouyangzhenyu@nbu.edu.cn`.

The authors declare that they have no known competing financial interests or
personal relationships that could have appeared to influence the work reported
in this paper.

## Acknowledgments

This work was supported by the National Natural Science Foundation of China
(Grant Nos. 12572288 and 12132015), the Zhejiang Provincial Natural Science
Foundation of China (Grant Nos. LR26A020004 and LQN25A020006), and the Ningbo
Municipal Natural Science Foundation of China (Grant No. 2025J075). The authors
thank Prof. Gaojin Li for helpful discussions. The numerical calculations were
performed at the Hefei Advanced Computing Center.

## Citation

If you use this code or dataset, please cite the associated manuscript:

> Chen Liu, Xiangyu Zhang, Zhenyu Ouyang, Jianzhong Lin, and Nhan Phan-Thien.
> *A geometry-driven transport crossover emerges across microbial propulsion
> modes.*

Please also cite the archived release:

> Liu, Chen, Zhang, Xiangyu, Ouyang, Zhenyu, Lin, Jianzhong, and Phan-Thien,
> Nhan. (2026). *A geometry-driven transport crossover emerges across microbial
> propulsion modes* (v1.0.0). Zenodo.
> https://doi.org/10.5281/zenodo.22043250

The stable DOI for all repository versions is
[10.5281/zenodo.22043249](https://doi.org/10.5281/zenodo.22043249). The journal
reference and manuscript DOI should be added here when available.

## License

The software developed for this project, including the bundled `funtrack-core`
wheel, and the documentation are licensed under the [MIT License](LICENSE).
The research dataset stored in `data/parts/` and its reconstructed contents are
licensed under [CC BY 4.0](data/LICENSE). External dependencies remain subject
to their own license terms.
