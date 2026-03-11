/**
 * @file V2XMessage.kt
 * @brief Kotlin data classes representing decoded V2X messages
 *
 * Mirrors C++ structures from v2x_structures.hpp for JNI marshalling.
 * Serializable for Android SharedPreferences, logging, and intent extras.
 *
 * Message Types Supported:
 *   - BSM (Basic Safety Message) - vehicle telemetry
 *   - SPaT (Signal Phase and Timing) - traffic signals
 *   - PSM (Personal Safety Message) - pedestrians/cyclists
 *
 * @author Sentinel V2X Bridge
 * @date March 11, 2026
 */

package com.sentinel.v2x

import java.io.Serializable

// ============================================================================
// Enumerations
// ============================================================================

/**
 * IEEE 1609.2 message frame types
 */
enum class MessageFrameType {
    BSM,              // Basic Safety Message
    SPAT,             // Signal Phase and Timing
    PSM,              // Personal Safety Message
    TIM,              // Traveler Information Message
    EVA,              // Emergency Vehicle Alert
    TCM,              // Traffic Control Message
    UNKNOWN           // Unrecognized frame type
}

/**
 * Vehicle classification per IEEE 1609.2
 */
enum class VehicleType {
    SEDAN,
    SUV,
    TRUCK,
    MOTORCYCLE,
    BUS,
    EMERGENCY,        // Police, fire, ambulance
    PASSENGER_CAR,
    UNKNOWN
}

/**
 * Safety alert types
 */
enum class AlertType {
    NONE,
    HARD_BRAKING,
    LANE_CHANGE,
    TURN,
    HAZARD,
    ACCIDENT,
    ROADWORK,
    BLIND_SPOT
}

// ============================================================================
// Position and Motion Data
// ============================================================================

/**
 * Geographic location in WGS84 coordinates
 *
 * @param latitude Degrees (-90.0 to +90.0), negative = South
 * @param longitude Degrees (-180.0 to +180.0), negative = West
 * @param elevation Meters above sea level
 * @param accuracy Position accuracy in meters (1σ uncertainty)
 */
data class GeoPosition(
    val latitude: Double = 0.0,
    val longitude: Double = 0.0,
    val elevation: Float = 0.0f,
    val accuracy: Float = 0.0f
) : Serializable

/**
 * Vehicle motion parameters
 *
 * @param speed Meters per second (>= 0)
 * @param heading Degrees (0-359), 0=North, 90=East
 * @param acceleration Meters/second² (-10 to +10)
 * @param yawRate Degrees per second (positive = left turn)
 */
data class Motion(
    val speed: Float = 0.0f,
    val heading: Float = 0.0f,
    val acceleration: Float = 0.0f,
    val yawRate: Float = 0.0f
) : Serializable

// ============================================================================
// BSM (Basic Safety Message)
// ============================================================================

/**
 * Decoded Basic Safety Message
 *
 * IEEE 1609.2-2016 BSM: Vehicle telemetry sent 10x/second
 * Contains position, motion, heading, speed, and safety alerts
 *
 * @param senderId Unique sender ID (MAC address format: "01:23:45:67:89:AB")
 * @param timestampMs Message generation timestamp (UNIX epoch milliseconds)
 * @param sequenceNum Sequence number (0-127, helps detect message loss)
 * @param position Geographic location
 * @param motion Speed, heading, acceleration, yaw rate
 * @param vehicleType Vehicle classification
 * @param activeAlerts List of currentlyactive safety warnings
 * @param vehicleInfo Additional vehicle metadata
 */
data class BasicSafetyMessage(
    val senderId: String = "",
    val timestampMs: Long = 0L,
    val sequenceNum: Int = 0,
    val position: GeoPosition = GeoPosition(),
    val motion: Motion = Motion(),
    val vehicleType: VehicleType = VehicleType.UNKNOWN,
    val activeAlerts: List<AlertType> = emptyList(),
    val vehicleInfo: VehicleInfo = VehicleInfo()
) : Serializable {
    /**
     * Additional vehicle metadata
     */
    data class VehicleInfo(
        val isEmergency: Boolean = false,
        val isEquippedWithEVCharging: Boolean = false,
        val isParked: Boolean = false,
        val wipersActive: Int = 0,      // 0=off, 1=low, 2=med, 3=high
        val doorsOpenCount: Int = 0,
        val wheelBase: Float = 2.7f,    // Distance (m) between front/rear axles
        val trackWidth: Float = 1.6f    // Distance (m) between left/right wheels
    ) : Serializable
}

// ============================================================================
// SPaT (Signal Phase and Timing)
// ============================================================================

/**
 * State of a single traffic signal phase
 *
 * @param intersectionId Unique intersection identifier
 * @param remId Road Regulatory Element ID (which lanes controlled)
 * @param currentPhase Current phase number (0-7)
 * @param timeInPhaseSec Seconds remaining in current phase (0-255)
 * @param confidencePercent Confidence in timing (0-100)
 * @param nextPhase Upcoming phase after current
 * @param timeToNextPhaseSec Estimated seconds until next phase (0-255)
 * @param allowsThroughTraffic Is through traffic currently permitted?
 * @param allowsLeftTurns Are left turns currently permitted?
 * @param allowsRightTurns Are right turns currently permitted?
 */
data class IntersectionState(
    val intersectionId: Long = 0L,
    val remId: Long = 0L,
    val currentPhase: Int = 0,
    val timeInPhaseSec: Int = 0,
    val confidencePercent: Int = 50,
    val nextPhase: Int = 0,
    val timeToNextPhaseSec: Int = 0,
    val allowsThroughTraffic: Boolean = false,
    val allowsLeftTurns: Boolean = false,
    val allowsRightTurns: Boolean = false
) : Serializable

/**
 * Decoded Signal Phase and Timing message
 *
 * IEEE 1609.2-2016 SPaT: Sent by traffic signal controllers at intersections
 * Allows vehicles to anticipate and optimize for signal changes
 *
 * @param timestampMs Message generation timestamp
 * @param intersections States of all controlled intersections
 * @param transmitterLocation Transmitter position for triangulation
 */
data class SignalPhaseAndTiming(
    val timestampMs: Long = 0L,
    val intersections: List<IntersectionState> = emptyList(),
    val transmitterLocation: GeoPosition = GeoPosition()
) : Serializable

// ============================================================================
// PSM (Personal Safety Message)
// ============================================================================

/**
 * Decoded Personal Safety Message
 *
 * IEEE 1609.2-2016 PSM: Sent by pedestrians, cyclists, mobility devices
 * Allows vehicles to detect vulnerable road users
 *
 * @param senderId Unique identifier (MAC address)
 * @param timestampMs Message generation timestamp
 * @param userType Type of user (0=pedestrian, 1=cyclist, 2=motorcycle, 3=scooter)
 * @param position Geographic location
 * @param heading Movement direction (degrees)
 * @param speed Movement speed (m/s)
 * @param alerts Safety-related flags
 */
data class PersonalSafetyMessage(
    val senderId: String = "",
    val timestampMs: Long = 0L,
    val userType: Int = 0,
    val position: GeoPosition = GeoPosition(),
    val heading: Float = 0.0f,
    val speed: Float = 0.0f,
    val alerts: Alerts = Alerts()
) : Serializable {
    /**
     * Alert flags for PSM
     */
    data class Alerts(
        val isHardBraking: Boolean = false,
        val hasfallen: Boolean = false,
        val isStationaryInRoadway: Boolean = false,
        val isSignalingTurn: Boolean = false
    ) : Serializable
}

// ============================================================================
// Decoded Message Container
// ============================================================================

/**
 * Container for any decoded V2X message
 *
 * After COER parsing and ASN.1 extraction, raw bytes are converted to
 * this sealed class hierarchy for type-safe Kotlin representation.
 *
 * Usage:
 * ```kotlin
 * val decoded: DecodedV2XMessage = v2x.processMessage(rawBytes)
 * when (decoded) {
 *     is DecodedV2XMessage.BSM -> {
 *         val lat = decoded.message.position.latitude
 *         val lon = decoded.message.position.longitude
 *         Log.d("V2X", "Vehicle at ($lat, $lon)")
 *     }
 *     is DecodedV2XMessage.SPaT -> {
 *         for (intersection in decoded.message.intersections) {
 *             Log.d("V2X", "Intersection ${intersection.intersectionId}: Phase ${intersection.currentPhase}")
 *         }
 *     }
 *     is DecodedV2XMessage.PSM -> {
 *         Log.d("V2X", "Pedestrian at ${decoded.message.position}")
 *     }
 *     is DecodedV2XMessage.Unknown -> {
 *         Log.w("V2X", "Unknown message type")
 *     }
 * }
 * ```
 */
sealed class DecodedV2XMessage : Serializable {
    /**
     * Timestamp when message was received (local system time, milliseconds)
     */
    abstract val receivedAtMs: Long
    
    /**
     * Was message cryptographically verified?
     */
    abstract val isVerified: Boolean
    
    /**
     * Sender certificate issuer (CA name), empty if unverified
     */
    abstract val issuerName: String
    
    /**
     * Human-readable frame type description
     */
    abstract fun frameTypeName(): String
    
    /**
     * Decoded Basic Safety Message
     */
    data class BSM(
        val message: BasicSafetyMessage,
        override val receivedAtMs: Long,
        override val isVerified: Boolean,
        override val issuerName: String
    ) : DecodedV2XMessage(), Serializable {
        override fun frameTypeName(): String = "BSM (Basic Safety Message)"
    }
    
    /**
     * Decoded Signal Phase and Timing message
     */
    data class SPaT(
        val message: SignalPhaseAndTiming,
        override val receivedAtMs: Long,
        override val isVerified: Boolean,
        override val issuerName: String
    ) : DecodedV2XMessage(), Serializable {
        override fun frameTypeName(): String = "SPaT (Signal Phase & Timing)"
    }
    
    /**
     * Decoded Personal Safety Message
     */
    data class PSM(
        val message: PersonalSafetyMessage,
        override val receivedAtMs: Long,
        override val isVerified: Boolean,
        override val issuerName: String
    ) : DecodedV2XMessage(), Serializable {
        override fun frameTypeName(): String = "PSM (Personal Safety Message)"
    }
    
    /**
     * Unknown or unsupported message type
     */
    data class Unknown(
        val frameTypeId: Int = -1,
        override val receivedAtMs: Long,
        override val isVerified: Boolean,
        override val issuerName: String,
        val errorMessage: String = ""
    ) : DecodedV2XMessage(), Serializable {
        override fun frameTypeName(): String = "UNKNOWN"
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Convert frame type enum to string
 */
fun MessageFrameType.toDisplayString(): String = when (this) {
    MessageFrameType.BSM -> "Basic Safety Message"
    MessageFrameType.SPAT -> "Signal Phase & Timing"
    MessageFrameType.PSM -> "Personal Safety Message"
    MessageFrameType.TIM -> "Traveler Information"
    MessageFrameType.EVA -> "Emergency Vehicle Alert"
    MessageFrameType.TCM -> "Traffic Control Message"
    MessageFrameType.UNKNOWN -> "Unknown Message Type"
}

/**
 * Convert vehicle type enum to string
 */
fun VehicleType.toDisplayString(): String = when (this) {
    VehicleType.SEDAN -> "Sedan"
    VehicleType.SUV -> "SUV"
    VehicleType.TRUCK -> "Truck"
    VehicleType.MOTORCYCLE -> "Motorcycle"
    VehicleType.BUS -> "Bus"
    VehicleType.EMERGENCY -> "Emergency Vehicle"
    VehicleType.PASSENGER_CAR -> "Passenger Car"
    VehicleType.UNKNOWN -> "Unknown Type"
}

/**
 * Convert alert type enum to string
 */
fun AlertType.toDisplayString(): String = when (this) {
    AlertType.NONE -> "No Alert"
    AlertType.HARD_BRAKING -> "Hard Braking"
    AlertType.LANE_CHANGE -> "Lane Change"
    AlertType.TURN -> "Sharp Turn"
    AlertType.HAZARD -> "Hazard Lights"
    AlertType.ACCIDENT -> "Accident"
    AlertType.ROADWORK -> "Roadwork"
    AlertType.BLIND_SPOT -> "Blind Spot"
}
