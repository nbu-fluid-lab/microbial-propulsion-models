# Numerical Simulation

This directory contains a two-dimensional numerical model for an active elliptical particle interacting with a field of circular obstacles. The implementation combines a D2Q9 lattice-Boltzmann fluid solver with immersed-boundary particle coupling and writes particle and flow-field data as plain text.

## Source files

- `main.cpp`: initialization, time stepping, collision handling, and output scheduling
- `cylinder.h`: domain constants, particle and obstacle data structures, memory allocation, and output helpers
- `utils.h`: vector, matrix, random-number, and geometry utilities
- `Makefile`: reproducible local build commands

## Requirements

- A C++17 compiler such as Clang or GCC
- POSIX-compatible shell and `make`
- Approximately 100 MB of memory for the default grid, plus disk space for generated flow fields

No third-party C++ library is required.

## Configure the model

The primary compile-time parameters are near the top of `cylinder.h`:

| Parameter | Meaning | Default |
| --- | --- | --- |
| `MM`, `NN` | Computational domain dimensions | `600`, `600` |
| `RR` | Radius of the outer obstacle-center ring | `125` |
| `OBSTACLE_RADIUS` | Circular obstacle radius | `7.5` |
| `ACTIVE_INIT_RADIUS` | Initial radial position of the active particle | `170.0` |
| `ACTIVE_TARGET_RADIUS` | Radius used to choose an inward initial direction | `0.5 * RR` |
| `FNum` | Total number of particles, including the active particle and obstacles | `4` |
| `Re` | Reynolds number | `0.01` |
| `AR`, `init_a` | Active-particle shape parameters | `0.72`, `5.8` |

The first particle is active, the second is the central obstacle, and the remaining `FNum - 2` particles form the outer obstacle ring. Record every changed constant with the corresponding run output.

The time-step upper bound and output intervals are in `main.cpp`. The default upper bound is intentionally very large; set an experiment-appropriate stopping condition before launching an unattended run.

## Build

```bash
make
```

The executable is written to `build/funtrack_simulation`. To enable compiler optimizations explicitly or use another compiler:

```bash
make CXX=g++ CXXFLAGS="-O3 -std=c++17 -Wall -Wextra"
```

## Run

Run each parameter set in its own working directory because output paths are relative to the current directory:

```bash
mkdir -p run
cd run
../build/funtrack_simulation
```

Stop a foreground run with `Ctrl-C`. For a bounded production run, change the loop limit in `main.cpp`, rebuild, and record that limit in the run metadata.

## Outputs

The simulation creates files such as:

- `orientation<N>.dat`: particle position, orientation, force, velocity, torque, and time
- `contact.dat`: contact information
- `particel_track.dat`: active-particle tracking history; the filename is retained for compatibility
- `01_flowfield/V<step>.dat`: density, velocity, vorticity, and pressure fields
- `02_particle_trajectory/*.dat`: normalized particle trajectories
- `03_particle_force/*.dat`: particle-boundary force fields
- `04_particle_profile/*.dat`: particle profiles

These files use whitespace-separated columns. Several field files include Tecplot-compatible headers.

## Clean build artifacts

```bash
make clean
```

This removes only the local `build/` directory. Simulation results in a run directory are not removed.

## Reproducibility notes

The active particle's initial position and inward direction are randomized from a clock-based seed. Exact reruns therefore require replacing the clock-based seeds in `main.cpp` and `utils.h` with recorded fixed seeds. Also archive the compiler version, source revision, parameter constants, stopping condition, and generated output together.
