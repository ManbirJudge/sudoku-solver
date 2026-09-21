plugins {
    alias(libs.plugins.android.application)
    // alias(libs.plugins.kotlin.android)
    // id("com.mikepenz.aboutlibraries.plugin")
}

android {
    namespace = "com.example.sudokusolver"
    compileSdk = 37

    defaultConfig {
        applicationId = "com.example.sudokusolver"

        minSdk = 24
        targetSdk = 37

        versionCode = 1
        versionName = "0.1"

        ndk {
            abiFilters += "arm64-v8a" // maybe support other architectures in the future?
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++11", "-frtti", "-fexceptions")
                arguments += listOf("-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    // kotlin {
    //     compilerOptions {
    //         jvmTarget = org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17
    //     }
    // }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        viewBinding = true
        prefab = true
        resValues = true
    }
}

dependencies {
    implementation(project(":opencv"))

    // implementation(libs.aboutlibraries)

    implementation(libs.androidx.camera.core)
    implementation(libs.androidx.camera.camera2)
    implementation(libs.androidx.camera.lifecycle)
    implementation(libs.androidx.camera.view)
    implementation(libs.androidx.camera.extensions)

    implementation(libs.androidx.preference)
    implementation(libs.androidx.navigation.fragment.ktx)
    implementation(libs.androidx.navigation.ui.ktx)
    implementation(libs.androidx.fragment.ktx)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.core.ktx)

    implementation(libs.material)

    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit.v130)
}