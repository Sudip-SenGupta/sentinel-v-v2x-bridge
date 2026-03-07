android {
    // ...

    defaultConfig {
        // ...
        externalNativeBuild {
            cmake {
                // Force the compiler to use C++17 for your security engine
                cppFlags("-std=c++17")
            }
        }

        ndk {
            abiFilters.add("x86_64")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../native-engine/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}