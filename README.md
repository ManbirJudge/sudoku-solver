# Sudoku Solver
## Structure
- `archive` - Contains some older files; now replaced or not needed.
- `benchmark` - Benchmarks for our solver. Includes `solver` as sub-project.
- `digit-model-trainer` - As the name says, digit model training script (Python + TensorFlow).
- `solver` - The solving algorithm.
- `SudokuSolver` - Android project - integrates `solver` and final model file from `digit-model-trainer`.
<!-- ## TODOs
- Error handling.
- Manual page.
- Calibration page.
- How to use page.
- Configuration.
  - More options.
  - Saving.
- Ads. -->
## Changelog
### September 2026
- Added guessing; can solve every valid Sudoku now (which it couldn't before).
- Updated the Android project (a lot; read `SudokuSolver/README.md`).
- A lot of other small/big things.
### 16-01-2026
- Merged both separate projects into one repository.
- Initialized Git and GitHub repositories.
### Some days after that
- Android app + computer-vision based recognition + solving
### 12-08-2025 02:00 
- First working algorithm (95% solvability) (~45 solutions/s)
### 11-08-2025 23:00 
- Started