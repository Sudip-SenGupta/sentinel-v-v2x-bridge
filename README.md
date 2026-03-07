graph TD
A[Automotive Android UI - Kotlin] -- JNI Bridge --> B[Sentinel-V Engine - C++17]
B --> C{Security Manager}
C -- Signature Valid --> D[V2X Message Dispatcher]
C -- Invalid --> E[Threat Alert Service]
D --> F[V2V / V2I Communication]