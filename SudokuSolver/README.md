# Sudoku Solver (Android)
## TODOs
- Don't ship the whole OpenCV and TFLite - provide instructions to get them.
## Updates
## 22-09-2026
- Migrated Gradle to 9.5.0 and AGP 9.3 with built-in Kotlin.
- Removed deprecated Gradle/AGP configuration and the AboutLibraries third-party package.
- Moved OpenCV and TensorFlow Lite dependencies outside `src`.
- Migrated OpenCV from 4.x to 5.0; added the required geometry header for API changes.
- Replaced the old TensorFlow Lite setup with the prebuilt 2.16.1 AAR runtime (from Maven Central) and matching TensorFlow headers (from TensorFlow repository).
- Fixed to make above changes work.