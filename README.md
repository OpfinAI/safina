# Safina

Safina is a research project for modeling crop water requirements using a modified Penman–Monteith equation, with a focus on drip irrigation scenarios. The codebase includes utilities for working with weather data in a custom binary format (GBIX), crop and environment modeling, and simulation routines.

## Project Structure

- `src/` — Source code for main simulation, GBIX utilities, and setup routines.
- `include/` — Header files for core data structures and function declarations.
- `data/` — Example weather data in CSV and GBIX formats.
- `papers/` — Research paper and documentation in LaTeX.
- `Makefile` — Build instructions for the project.

## Building

To build the project, run:

```sh
make
```

## License

MIT License
