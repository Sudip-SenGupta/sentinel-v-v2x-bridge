plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.sentinel.v2x"
    compileSdk = 34
    ndkVersion = "25.0.8775105"

    defaultConfig {
        applicationId = "com.sentinel.v2x"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        
        externalNativeBuild {
            cmake {
                cppFlags("-std=c++17")
                arguments("-DANDROID_STL=c++_shared")
            }
        }

        ndk {
            abiFilters.add("x86_64")
            abiFilters.add("arm64-v8a")
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
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    
    lint {
        disable.add("MissingDimensionBaseline")
        disable.add("MissingDimensionAndroidTest") 
        abortOnError = false
        checkReleaseBuilds = false
    }
    
    externalNativeBuild {
        cmake {
            path = file("../native-engine/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation(project(":android-app"))
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}
