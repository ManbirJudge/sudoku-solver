# Sudoku Solver (Android)
## Build
- Download OpenCV 5.0.0 Android SDK and extract it to `third_party/opencv`
  - [OpenCV Release Page](https://opencv.org/releases/)
  - [Direct Download](https://github.com/opencv/opencv/releases/download/5.0.0/opencv-5.0.0-android-sdk.zip)
## TODOs
- Migrate from TFLite 2.17.x to latest LiteRT.
- Don't ship the whole TFLite; provide instructions to get it.
## Updates
## 22-09-2026
- Migrated Gradle to 9.5.0 and AGP 9.3 with built-in Kotlin.
- Removed deprecated Gradle/AGP configuration and the AboutLibraries third-party package.
- Moved OpenCV and TensorFlow Lite dependencies outside `src`.
- Migrated OpenCV from 4.x to 5.0; added the required geometry header for API changes.
- Replaced the old TensorFlow Lite setup with the prebuilt 2.16.1 AAR runtime (from Maven Central) and matching TensorFlow headers (from TensorFlow repository).
- Updated the project configuration and native build to support these changes.