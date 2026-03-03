android {
    // ... other config
    externalNativeBuild {
        cmake {
            path = file("../native-engine/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    
    defaultConfig {
        ndk {
            // Target specific architectures for the AAOS Emulator (usually x86_64)
            abiFilters.add("x86_64") 
        }
    }
}