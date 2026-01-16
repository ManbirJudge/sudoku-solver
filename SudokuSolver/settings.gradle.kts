pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "Sudoku Solver"

include(":app")
include(":opencv")

project(":opencv").projectDir = File(rootDir, "app/src/main/cpp/opencv/sdk/")