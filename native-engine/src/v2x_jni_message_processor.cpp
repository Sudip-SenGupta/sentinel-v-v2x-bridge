/**
 * @file v2x_jni_message_processor.cpp
 * @brief JNI Bridge for V2X Message Processing
 *
 * Marshals C++ DecodedV2XMessage structures to Java/Kotlin objects.
 * Enables Android apps to process real V2X data through the cipher.
 *
 * Phase 4 Delivery: Full message processing pipeline via JNI
 *
 * @author Sentinel V2X Bridge
 * @date March 11, 2026
 */

#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include "v2x_coer_decoder.h"
#include "v2x_frame_decoder.h"
#include "v2x_message_processor.h"
#include "v2x_structures.hpp"

#define TAG "V2X-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

using namespace sentinel::v2x;

// ============================================================================
// JNI Helper: Java Class References
// ============================================================================

class JavaClasses {
public:
    // Message-related classes
    static jclass MessageFrameType;
    static jclass DecodedV2XMessage_BSM;
    static jclass DecodedV2XMessage_SPaT;
    static jclass DecodedV2XMessage_PSM;
    static jclass DecodedV2XMessage_Unknown;

    // Data structure classes
    static jclass BasicSafetyMessage;
    static jclass SignalPhaseAndTiming;
    static jclass PersonalSafetyMessage;
    static jclass GeoPosition;
    static jclass Motion;
    static jclass IntersectionState;
    static jclass VehicleType;

    // Primitive wrappers
    static jclass Integer;
    static jclass Long;
    static jclass Float;

    // Collections
    static jclass ArrayList;
};

// Static initialization (needed to cache class references)
jclass JavaClasses::MessageFrameType = nullptr;
jclass JavaClasses::DecodedV2XMessage_BSM = nullptr;
jclass JavaClasses::DecodedV2XMessage_SPaT = nullptr;
jclass JavaClasses::DecodedV2XMessage_PSM = nullptr;
jclass JavaClasses::DecodedV2XMessage_Unknown = nullptr;
jclass JavaClasses::BasicSafetyMessage = nullptr;
jclass JavaClasses::SignalPhaseAndTiming = nullptr;
jclass JavaClasses::PersonalSafetyMessage = nullptr;
jclass JavaClasses::GeoPosition = nullptr;
jclass JavaClasses::Motion = nullptr;
jclass JavaClasses::IntersectionState = nullptr;
jclass JavaClasses::VehicleType = nullptr;
jclass JavaClasses::Integer = nullptr;
jclass JavaClasses::Long = nullptr;
jclass JavaClasses::Float = nullptr;
jclass JavaClasses::ArrayList = nullptr;

// ============================================================================
// JNI Helper: Initialize Class References
// ============================================================================

bool initializeJavaClasses(JNIEnv* env) {
    try {
        // Find classes
        JavaClasses::GeoPosition = env->FindClass("com/sentinel/v2x/GeoPosition");
        JavaClasses::Motion = env->FindClass("com/sentinel/v2x/Motion");
        JavaClasses::BasicSafetyMessage = env->FindClass("com/sentinel/v2x/BasicSafetyMessage");
        JavaClasses::SignalPhaseAndTiming = env->FindClass("com/sentinel/v2x/SignalPhaseAndTiming");
        JavaClasses::PersonalSafetyMessage = env->FindClass("com/sentinel/v2x/PersonalSafetyMessage");
        JavaClasses::IntersectionState = env->FindClass("com/sentinel/v2x/IntersectionState");
        JavaClasses::VehicleType = env->FindClass("com/sentinel/v2x/VehicleType");
        JavaClasses::DecodedV2XMessage_BSM = env->FindClass("com/sentinel/v2x/DecodedV2XMessage$BSM");
        JavaClasses::DecodedV2XMessage_SPaT = env->FindClass("com/sentinel/v2x/DecodedV2XMessage$SPaT");
        JavaClasses::DecodedV2XMessage_PSM = env->FindClass("com/sentinel/v2x/DecodedV2XMessage$PSM");
        JavaClasses::DecodedV2XMessage_Unknown = env->FindClass("com/sentinel/v2x/DecodedV2XMessage$Unknown");
        JavaClasses::ArrayList = env->FindClass("java/util/ArrayList");

        // Check for errors
        if (env->ExceptionCheck()) {
            LOGE("Failed to find Java classes");
            env->ExceptionDescribe();
            return false;
        }

        // Make global references (required for use across JNI calls)
        JavaClasses::GeoPosition = (jclass)env->NewGlobalRef(JavaClasses::GeoPosition);
        JavaClasses::Motion = (jclass)env->NewGlobalRef(JavaClasses::Motion);
        JavaClasses::BasicSafetyMessage = (jclass)env->NewGlobalRef(JavaClasses::BasicSafetyMessage);
        JavaClasses::SignalPhaseAndTiming = (jclass)env->NewGlobalRef(JavaClasses::SignalPhaseAndTiming);
        JavaClasses::PersonalSafetyMessage = (jclass)env->NewGlobalRef(JavaClasses::PersonalSafetyMessage);
        JavaClasses::IntersectionState = (jclass)env->NewGlobalRef(JavaClasses::IntersectionState);
        JavaClasses::VehicleType = (jclass)env->NewGlobalRef(JavaClasses::VehicleType);
        JavaClasses::DecodedV2XMessage_BSM = (jclass)env->NewGlobalRef(JavaClasses::DecodedV2XMessage_BSM);
        JavaClasses::DecodedV2XMessage_SPaT = (jclass)env->NewGlobalRef(JavaClasses::DecodedV2XMessage_SPaT);
        JavaClasses::DecodedV2XMessage_PSM = (jclass)env->NewGlobalRef(JavaClasses::DecodedV2XMessage_PSM);
        JavaClasses::DecodedV2XMessage_Unknown = (jclass)env->NewGlobalRef(JavaClasses::DecodedV2XMessage_Unknown);
        JavaClasses::ArrayList = (jclass)env->NewGlobalRef(JavaClasses::ArrayList);

        LOGI("Java classes initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Exception during Java class initialization: %s", e.what());
        return false;
    }
}

// ============================================================================
// JNI Helper: Convert C++ to Java
// ============================================================================

/**
 * Create Java GeoPosition from C++ GeoPosition
 */
jobject createJavaGeoPosition(JNIEnv* env, const GeoPosition& pos) {
    jmethodID constructor = env->GetMethodID(
        JavaClasses::GeoPosition, "<init>",
        "(DDFF)V"  // double lat, double lon, float elev, float acc
    );

    if (!constructor) {
        LOGE("GeoPosition constructor not found");
        return nullptr;
    }

    return env->NewObject(
        JavaClasses::GeoPosition,
        constructor,
        pos.latitude,
        pos.longitude,
        pos.elevation,
        pos.accuracy
    );
}

/**
 * Create Java Motion from C++ Motion
 */
jobject createJavaMotion(JNIEnv* env, const Motion& motion) {
    jmethodID constructor = env->GetMethodID(
        JavaClasses::Motion, "<init>",
        "(FFFF)V"  // speed, heading, accel, yaw_rate
    );

    if (!constructor) {
        LOGE("Motion constructor not found");
        return nullptr;
    }

    return env->NewObject(
        JavaClasses::Motion,
        constructor,
        motion.speed,
        motion.heading,
        motion.acceleration,
        motion.yaw_rate
    );
}

/**
 * Convert C++ VehicleType to Java VehicleType enum
 * Maps C++ enum values to Java static field references
 */
jobject createJavaVehicleType(JNIEnv* env, VehicleType vehicleType) {
    const char* fieldName = nullptr;

    // Map C++ enum to Java enum field name
    switch (vehicleType) {
        case VehicleType::SEDAN:
            fieldName = "SEDAN";
            break;
        case VehicleType::SUV:
            fieldName = "SUV";
            break;
        case VehicleType::TRUCK:
            fieldName = "TRUCK";
            break;
        case VehicleType::MOTORCYCLE:
            fieldName = "MOTORCYCLE";
            break;
        case VehicleType::BUS:
            fieldName = "BUS";
            break;
        case VehicleType::EMERGENCY:
            fieldName = "EMERGENCY";
            break;
        case VehicleType::PASSENGER_CAR:
            fieldName = "PASSENGER_CAR";
            break;
        case VehicleType::UNKNOWN:
        default:
            fieldName = "UNKNOWN";
            break;
    }

    if (!fieldName) {
        LOGE("Unknown VehicleType value: %d", static_cast<int>(vehicleType));
        return nullptr;
    }

    // Get static field ID for the enum value
    jfieldID fieldID = env->GetStaticFieldID(
        JavaClasses::VehicleType,
        fieldName,
        "Lcom/sentinel/v2x/VehicleType;"
    );

    if (!fieldID) {
        LOGE("Failed to find VehicleType field: %s", fieldName);
        env->ExceptionClear();
        return nullptr;
    }

    // Get the enum constant value
    jobject enumValue = env->GetStaticObjectField(
        JavaClasses::VehicleType,
        fieldID
    );

    if (!enumValue) {
        LOGE("Failed to get VehicleType enum value: %s", fieldName);
        return nullptr;
    }

    LOGI("Created VehicleType enum: %s", fieldName);
    return enumValue;
}

/**
 * Create Java VehicleInfo from default values
 *
 * Uses Kotlin data class default constructor with all parameters
 * Kotlin compiles this to: (Z Z Z I I F F) V
 */
jobject createJavaVehicleInfo(JNIEnv* env) {
    // VehicleInfo is a nested data class within BasicSafetyMessage
    jclass vehicleInfoClass = env->FindClass("com/sentinel/v2x/BasicSafetyMessage$VehicleInfo");
    if (!vehicleInfoClass) {
        LOGE("VehicleInfo class not found");
        return nullptr;
    }

    // Try full constructor first: (boolean, boolean, boolean, int, int, float, float)
    jmethodID constructor = env->GetMethodID(
        vehicleInfoClass, "<init>",
        "(ZZZIIIFF)V"  // Correct signature with all 7 params
    );

    if (!constructor) {
        env->ExceptionClear();
        // Try no-arg constructor with defaults
        constructor = env->GetMethodID(vehicleInfoClass, "<init>", "()V");
        if (!constructor) {
            LOGE("VehicleInfo constructor not found (tried both full and no-arg)");
            env->DeleteLocalRef(vehicleInfoClass);
            return nullptr;
        }

        // Use default constructor
        jobject vehicleInfo = env->NewObject(vehicleInfoClass, constructor);
        LOGI("Created VehicleInfo with default constructor");
        env->DeleteLocalRef(vehicleInfoClass);
        return vehicleInfo;
    }

    // Use full constructor with explicit values
    jobject vehicleInfo = env->NewObject(
        vehicleInfoClass,
        constructor,
        (jboolean)false,     // isEmergency
        (jboolean)false,     // isEquippedWithEVCharging
        (jboolean)false,     // isParked
        (jint)0,             // wipersActive
        (jint)0,             // doorsOpenCount
        (jfloat)2.7f,        // wheelBase
        (jfloat)1.6f         // trackWidth
    );

    LOGI("Created VehicleInfo with full constructor");
    env->DeleteLocalRef(vehicleInfoClass);
    return vehicleInfo;
}

/**
 * Create Java BasicSafetyMessage from C++ BSM
 */
jobject createJavaBasicSafetyMessage(JNIEnv* env, const BasicSafetyMessage& bsm) {
    // Create position and motion objects first
    jobject javaPos = createJavaGeoPosition(env, bsm.position);
    jobject javaMotion = createJavaMotion(env, bsm.motion);
    jobject javaVehicleType = createJavaVehicleType(env, bsm.vehicle_type);
    jobject javaVehicleInfo = createJavaVehicleInfo(env);

    if (!javaPos || !javaMotion || !javaVehicleType || !javaVehicleInfo) {
        LOGE("Failed to create position, motion, vehicle type, or vehicle info objects");
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(
        JavaClasses::BasicSafetyMessage, "<init>",
        "(Ljava/lang/String;JILcom/sentinel/v2x/GeoPosition;"
        "Lcom/sentinel/v2x/Motion;Lcom/sentinel/v2x/VehicleType;"
        "Ljava/util/List;Lcom/sentinel/v2x/BasicSafetyMessage$VehicleInfo;)V"
    );

    if (!constructor) {
        LOGE("BasicSafetyMessage constructor not found");
        return nullptr;
    }

    // Create sender_id string
    jstring jsenderId = env->NewStringUTF(bsm.sender_id.c_str());

    // Create empty alert list for now (simplified; full implementation would populate this)
    jobject javaAlerts = env->NewObject(JavaClasses::ArrayList,
        env->GetMethodID(JavaClasses::ArrayList, "<init>", "()V"));

    jobject bsm_object = env->NewObject(
        JavaClasses::BasicSafetyMessage,
        constructor,
        jsenderId,
        (jlong)bsm.timestamp_ms,
        (jint)bsm.sequence_num,
        javaPos,
        javaMotion,
        javaVehicleType,
        javaAlerts,
        javaVehicleInfo
    );

    env->DeleteLocalRef(jsenderId);
    env->DeleteLocalRef(javaPos);
    env->DeleteLocalRef(javaMotion);
    env->DeleteLocalRef(javaAlerts);
    env->DeleteLocalRef(javaVehicleInfo);

    return bsm_object;
}

/**
 * Create Java DecodedV2XMessage.BSM from C++ DecodedV2XMessage
 */
jobject createJavaDecodedBSM(JNIEnv* env, const DecodedV2XMessage& decoded) {
    try {
        jobject message = createJavaBasicSafetyMessage(env, std::get<BasicSafetyMessage>(decoded.payload));
        if (!message) return nullptr;

        jmethodID constructor = env->GetMethodID(
            JavaClasses::DecodedV2XMessage_BSM, "<init>",
            "(Lcom/sentinel/v2x/BasicSafetyMessage;JZLjava/lang/String;)V"
        );

        if (!constructor) {
            LOGE("DecodedV2XMessage.BSM constructor not found");
            return nullptr;
        }

        jstring jissuer = env->NewStringUTF(decoded.issuer_name.c_str());

        jobject result = env->NewObject(
            JavaClasses::DecodedV2XMessage_BSM,
            constructor,
            message,
            (jlong)decoded.received_at_ms,
            (jboolean)decoded.is_verified,
            jissuer
        );

        env->DeleteLocalRef(jissuer);
        env->DeleteLocalRef(message);

        return result;
    } catch (const std::exception& e) {
        LOGE("Exception in createJavaDecodedBSM: %s", e.what());
        return nullptr;
    }
}

// ============================================================================
// JNI Exported Functions
// ============================================================================

extern "C" {

/**
 * JNI_OnLoad: Called when native library is loaded
 * Initialize Java class references here
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    JNIEnv* env = nullptr;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get JNI environment");
        return JNI_ERR;
    }

    // Initialize Java class references
    if (!initializeJavaClasses(env)) {
        LOGE("Failed to initialize Java classes");
        return JNI_ERR;
    }

    LOGI("V2X JNI library loaded successfully");
    return JNI_VERSION_1_6;
}

/**
 * Detect V2X message frame type from COER bytes
 *
 * Java signature:
 *   public static native String detectFrameType(byte[] coerBytes);
 */
JNIEXPORT jstring JNICALL Java_com_sentinel_v2x_V2X_detectFrameType(
    JNIEnv* env, jclass /*clazz*/, jbyteArray coerBytes
) {
    try {
        // Convert Java byte array to C++ vector
        jsize len = env->GetArrayLength(coerBytes);
        jbyte* data = env->GetByteArrayElements(coerBytes, nullptr);

        std::vector<uint8_t> payload(data, data + len);
        env->ReleaseByteArrayElements(coerBytes, data, JNI_ABORT);

        // Parse COER message
        COERMessage msg = COERDecoder::parse(payload);
        const auto& decoded_payload = COERDecoder::get_payload(msg);

        // Detect frame type
        MessageFrameType frame_type = V2XFrameDecoder::detect_frame_type(decoded_payload);

        // Convert to string
        std::string frame_str = V2XFrameDecoder::frame_type_to_string(frame_type);

        return env->NewStringUTF(frame_str.c_str());
    } catch (const std::exception& e) {
        LOGE("Frame type detection failed: %s", e.what());
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
        return env->NewStringUTF("ERROR");
    }
}

/**
 * Process complete V2X message from COER bytes
 *
 * Java signature:
 *   public static native DecodedV2XMessage processMessage(byte[] coerBytes);
 */
JNIEXPORT jobject JNICALL Java_com_sentinel_v2x_V2X_processMessage(
    JNIEnv* env, jclass /*clazz*/, jbyteArray coerBytes
) {
    try {
        // Convert Java byte array to C++ vector
        jsize len = env->GetArrayLength(coerBytes);
        jbyte* data = env->GetByteArrayElements(coerBytes, nullptr);

        std::vector<uint8_t> coer_data(data, data + len);
        env->ReleaseByteArrayElements(coerBytes, data, JNI_ABORT);

        LOGI("Processing COER message (%zu bytes)", coer_data.size());

        // Enforce the native verification pipeline before JNI marshalling.
        const auto verification = V2XMessageProcessor::process_message(coer_data);
        if (!verification.is_valid) {
            const std::string error = verification.error_message.empty()
                ? "Message verification failed"
                : verification.error_message;
            LOGE("Message processing failed verification: %s", error.c_str());
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"), error.c_str());
            return nullptr;
        }

        if (!verification.decoded_message.has_value()) {
            LOGE("Message verification succeeded without a decoded message");
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                         "Verified message is missing decoded payload");
            return nullptr;
        }

        const auto& decoded = *verification.decoded_message;
        const auto frame_type = verification.frame_type;
        LOGI("Using processor-decoded frame type: %s", V2XFrameDecoder::frame_type_to_string(frame_type).c_str());

        // Step 1: Marshal verified decoded data to Java object
        jobject result = nullptr;

        switch (frame_type) {
            case MessageFrameType::BSM:
                LOGI("Creating Java BSM object");
                result = createJavaDecodedBSM(env, decoded);
                break;

            case MessageFrameType::SPAT:
                LOGI("SPaT message type not yet fully supported in JNI");
                // TODO: Implement SPaT marshalling
                break;

            case MessageFrameType::PSM:
                LOGI("PSM message type not yet fully supported in JNI");
                // TODO: Implement PSM marshalling
                break;

            default:
                LOGI("Unknown message type");
                break;
        }

        if (result == nullptr) {
            LOGE("Failed to create Java object");
            env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                         "Failed to marshal message to Java object");
        }

        return result;
    } catch (const COERFormatException& e) {
        LOGE("COER format error: %s", e.what());
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
        return nullptr;
    } catch (const std::exception& e) {
        LOGE("Message processing failed: %s", e.what());
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
        return nullptr;
    }
}

/**
 * Process multiple V2X messages in batch
 *
 * Java signature:
 *   public static native List<DecodedV2XMessage> processBatch(List<byte[]> messages);
 */
JNIEXPORT jobject JNICALL Java_com_sentinel_v2x_V2X_processBatch(
    JNIEnv* env, jclass /*clazz*/, jobject messageList
) {
    try {
        if (messageList == nullptr) {
            env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                         "Message list cannot be null");
            return nullptr;
        }

        // Get ArrayList methods
        jclass listClass = env->GetObjectClass(messageList);
        jmethodID sizeMethodID = env->GetMethodID(listClass, "size", "()I");
        jmethodID getMethodID = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

        // Get size of input list
        jint listSize = env->CallIntMethod(messageList, sizeMethodID);
        LOGI("Processing batch of %d messages", listSize);

        // Create result ArrayList
        jobject resultList = env->NewObject(
            JavaClasses::ArrayList,
            env->GetMethodID(JavaClasses::ArrayList, "<init>", "()V")
        );

        jmethodID addMethodID = env->GetMethodID(JavaClasses::ArrayList, "add", "(Ljava/lang/Object;)Z");

        // Process each message in the batch
        for (jint i = 0; i < listSize; ++i) {
            try {
                // Get the i-th ByteArray from the list
                jobject messageObj = env->CallObjectMethod(messageList, getMethodID, i);

                if (messageObj == nullptr) {
                    LOGI("Message %d is null, skipping", i);
                    continue;
                }

                // Cast to byte array and process
                jbyteArray coerBytes = static_cast<jbyteArray>(messageObj);
                jsize len = env->GetArrayLength(coerBytes);
                jbyte* data = env->GetByteArrayElements(coerBytes, nullptr);

                std::vector<uint8_t> coer_data(data, data + len);
                env->ReleaseByteArrayElements(coerBytes, data, JNI_ABORT);

                LOGI("Processing message %d (%zu bytes)", i, coer_data.size());

                const auto verification = V2XMessageProcessor::process_message(coer_data);
                if (!verification.is_valid) {
                    const std::string error = verification.error_message.empty()
                        ? "Message verification failed"
                        : verification.error_message;
                    LOGE("Batch message %d failed verification: %s", i, error.c_str());
                    env->DeleteLocalRef(messageObj);
                    continue;
                }

                if (!verification.decoded_message.has_value()) {
                    LOGE("Batch message %d verified without a decoded message", i);
                    env->DeleteLocalRef(messageObj);
                    continue;
                }

                const auto& decoded = *verification.decoded_message;
                const auto frame_type = verification.frame_type;
                LOGI("Using processor-decoded frame type %s for message %d",
                     V2XFrameDecoder::frame_type_to_string(frame_type).c_str(), i);

                // Step 1: Marshal verified decoded data to Java object
                jobject javaMessage = nullptr;

                switch (frame_type) {
                    case MessageFrameType::BSM:
                        LOGI("Creating Java BSM object for batch message %d", i);
                        javaMessage = createJavaDecodedBSM(env, decoded);
                        break;

                    case MessageFrameType::SPAT:
                        LOGI("SPaT message not yet fully supported");
                        break;

                    case MessageFrameType::PSM:
                        LOGI("PSM message not yet fully supported");
                        break;

                    default:
                        LOGI("Unknown message type in batch");
                        break;
                }

                if (javaMessage != nullptr) {
                    env->CallBooleanMethod(resultList, addMethodID, javaMessage);
                    env->DeleteLocalRef(javaMessage);
                    LOGI("Added decoded message %d to result list", i);
                }

                env->DeleteLocalRef(messageObj);

            } catch (const std::exception& e) {
                LOGE("Error processing batch message %d: %s", i, e.what());
                // Continue processing remaining messages
                continue;
            }
        }

        LOGI("Batch processing complete: %d messages processed", listSize);
        return resultList;

    } catch (const std::exception& e) {
        LOGE("Batch processing failed: %s", e.what());
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
        return nullptr;
    }
}

}  // extern "C"
