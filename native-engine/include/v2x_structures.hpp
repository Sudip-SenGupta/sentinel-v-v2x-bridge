/**
 * @file v2x_structures.hpp
 * @brief V2X Message Data Structures
 * 
 * Defines the decoded representation of IEEE 1609.2 V2X messages
 * after COER parsing and ASN.1 extraction.
 * 
 * Supports:
 *   - BSM (Basic Safety Message) - Position, speed, heading
 *   - SPaT (Signal Phase and Timing) - Traffic light states
 *   - PSM (Personal Safety Message) - Pedestrian/cyclist alerts
 * 
 * Phase 4 Delivery: Structured data representation of V2X payloads
 * 
 * @author Sentinel V2X Bridge
 * @date March 11, 2026
 * @version 1.0.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <variant>

namespace sentinel::v2x {

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @enum MessageFrameType
 * @brief IEEE 1609.2 supported message types
 */
enum class MessageFrameType : uint8_t {
    BSM              = 0x01,  /** Basic Safety Message (vehicle telemetry) */
    SPAT             = 0x02,  /** Signal Phase and Timing (traffic lights) */
    PSM              = 0x03,  /** Personal Safety Message (pedestrians) */
    TIM              = 0x04,  /** Traveler Information Message */
    EVA              = 0x05,  /** Emergency Vehicle Alert */
    TCM              = 0x06,  /** Traffic Control Message */
    UNKNOWN          = 0xFF   /** Unrecognized frame type */
};

/**
 * @enum VehicleType
 * @brief IEEE 1609.2 vehicle classification
 */
enum class VehicleType : uint8_t {
    SEDAN            = 0x00,
    SUV              = 0x01,
    TRUCK            = 0x02,
    MOTORCYCLE       = 0x03,
    BUS              = 0x04,
    EMERGENCY        = 0x05,  /** Police, fire, ambulance */
    PASSENGER_CAR    = 0x06,
    UNKNOWN          = 0xFF
};

/**
 * @enum AlertType
 * @brief Safety alert classifications
 */
enum class AlertType : uint8_t {
    NONE              = 0x00,  /** No alert */
    HARD_BRAKING      = 0x01,  /** Severe deceleration */
    LANE_CHANGE       = 0x02,  /** Abrupt lane change */
    TURN              = 0x03,  /** Sharp turn */
    HAZARD            = 0x04,  /** Hazard lights on */
    ACCIDENT          = 0x05,  /** Accident detected */
    ROADWORK          = 0x06,  /** Road construction/maintenance */
    BLIND_SPOT        = 0x07,  /** Entering blind spot */
};

// ============================================================================
// Position and Motion Data
// ============================================================================

/**
 * @struct GeoPosition
 * @brief Geographic location in WGS84 coordinates
 */
struct GeoPosition {
    /**
     * Latitude in degrees (-90.0 to +90.0)
     * Negative = South, Positive = North
     */
    double latitude = 0.0;
    
    /**
     * Longitude in degrees (-180.0 to +180.0)
     * Negative = West, Positive = East
     */
    double longitude = 0.0;
    
    /**
     * Elevation in meters above sea level
     * Optional (may be 0 if unavailable)
     */
    float elevation = 0.0f;
    
    /**
     * Position accuracy in meters (1σ uncertainty)
     * Typical values: 5-50 meters for GPS
     */
    float accuracy = 0.0f;
};

/**
 * @struct Motion
 * @brief Vehicle motion parameters
 */
struct Motion {
    /**
     * Speed in meters per second (always >= 0)
     */
    float speed = 0.0f;
    
    /**
     * Heading in degrees (0-359)
     *   0° = North
     *  90° = East
     * 180° = South
     * 270° = West
     */
    float heading = 0.0f;
    
    /**
     * Acceleration magnitude in m/s²
     * Positive = accelerating
     * Negative = decelerating
     * Range: -10.0 to +10.0 m/s²
     */
    float acceleration = 0.0f;
    
    /**
     * Yaw rate (turning speed) in degrees per second
     * Positive = turning left
     * Negative = turning right
     */
    float yaw_rate = 0.0f;
};

// ============================================================================
// BSM (Basic Safety Message)
// ============================================================================

/**
 * @struct BasicSafetyMessage
 * @brief Decoded BSM payload
 * 
 * IEEE 1609.2-2016 Basic Safety Message (BSM)
 * Sent 10 times per second by vehicles (~100 ms interval)
 * Contains position, motion, heading, speed, and alerts
 */
struct BasicSafetyMessage {
    /**
     * Unique sender ID (MAC address of OBU)
     * Format: 6 bytes (e.g., "01:23:45:67:89:AB")
     */
    std::string sender_id;
    
    /**
     * Message timestamp (UNIX epoch)
     * When vehicle generated this message
     */
    uint64_t timestamp_ms = 0;
    
    /**
     * Sequence number (wraps every 128 messages)
     * Helps detect message loss
     */
    uint8_t sequence_num = 0;
    
    /**
     * Geographic position
     */
    GeoPosition position;
    
    /**
     * Motion parameters
     */
    Motion motion;
    
    /**
     * Vehicle type/classification
     */
    VehicleType vehicle_type = VehicleType::UNKNOWN;
    
    /**
     * Active safety alerts
     */
    std::vector<AlertType> active_alerts;
    
    /**
     * Additional metadata
     */
    struct {
        bool is_emergency = false;      /** Emergency lights active */
        bool is_equipped_with_ev_charging = false;
        bool is_parked = false;
        uint8_t wipers_active = 0;      /** 0=off, 1=low, 2=med, 3=high */
        uint8_t doors_open_count = 0;
        float wheel_base = 2.7f;        /** Distance (m) between front/rear axles */
        float track_width = 1.6f;       /** Distance (m) between left/right wheels */
    } vehicle_info;
};

// ============================================================================
// SPaT (Signal Phase and Timing)
// ============================================================================

/**
 * @struct IntersectionState
 * @brief State of a single traffic signal phase
 */
struct IntersectionState {
    /**
     * Intersection ID (unique in region)
     */
    uint32_t intersection_id = 0;
    
    /**
     * Road Regulatory Element (REM) ID
     * Identifies which lanes this signal controls
     */
    uint32_t rem_id = 0;
    
    /**
     * Current phase (0-7)
     * Maps to NEMA phases or custom definitions
     */
    uint8_t current_phase = 0;
    
    /**
     * Seconds remaining in current phase
     * Range: 0-255 seconds
     */
    uint8_t time_in_phase_sec = 0;
    
    /**
     * Confidence in timing (0=low, 100=high)
     */
    uint8_t confidence_percent = 50;
    
    /**
     * Next phase after current
     */
    uint8_t next_phase = 0;
    
    /**
     * Estimated seconds until next phase
     */
    uint8_t time_to_next_phase_sec = 0;
    
    /**
     * Is phase currently permitting through traffic?
     */
    bool allows_through_traffic = false;
    
    /**
     * Is phase currently permitting left turns?
     */
    bool allows_left_turns = false;
    
    /**
     * Is phase currently permitting right turns?
     */
    bool allows_right_turns = false;
};

/**
 * @struct SignalPhaseAndTiming
 * @brief Decoded SPaT payload
 * 
 * IEEE 1609.2-2016 Signal Phase and Timing (SPaT)
 * Sent by traffic signal controllers at intersections
 * Allows vehicles to anticipate signal changes
 */
struct SignalPhaseAndTiming {
    /**
     * Intersection clock (synchronized across region)
     */
    uint64_t timestamp_ms = 0;
    
    /**
     * States of all controlled intersections in region
     */
    std::vector<IntersectionState> intersections;
    
    /**
     * Transmitter location (for triangulation)
     */
    GeoPosition transmitter_location;
};

// ============================================================================
// PSM (Personal Safety Message)
// ============================================================================

/**
 * @struct PersonalSafetyMessage
 * @brief Decoded PSM payload
 * 
 * IEEE 1609.2-2016 Personal Safety Message (PSM)
 * Sent by pedestrians, cyclists, mobility devices with V2X radio
 * Allows vehicles to detect vulnerable road users
 */
struct PersonalSafetyMessage {
    /**
     * Unique identifier (MAC address)
     */
    std::string sender_id;
    
    /**
     * Message timestamp
     */
    uint64_t timestamp_ms = 0;
    
    /**
     * User type
     * 0 = pedestrian
     * 1 = cyclist
     * 2 = motorcycle
     * 3 = scooter/skateboard
     */
    uint8_t user_type = 0;
    
    /**
     * Position
     */
    GeoPosition position;
    
    /**
     * Movement direction (heading in degrees)
     */
    float heading = 0.0f;
    
    /**
     * Movement speed
     */
    float speed = 0.0f;
    
    /**
     * Alert flags
     */
    struct {
        bool is_hard_braking = false;
        bool has_fallen = false;
        bool is_stationary_in_roadway = false;
        bool is_signaling_turn = false;
    } alerts;
};

// ============================================================================
// Decoded Message Container
// ============================================================================

/**
 * @struct DecodedV2XMessage
 * @brief Container for any decoded V2X message
 * 
 * After COER parsing and ASN.1 extraction, raw bytes are converted to
 * this structured format for application use.
 */
struct DecodedV2XMessage {
    /**
     * Message type identifier
     */
    MessageFrameType frame_type = MessageFrameType::UNKNOWN;
    
    /**
     * Was message cryptographically verified?
     */
    bool is_verified = false;
    
    /**
     * Sender certificate issuer (CA name)
     * Empty if unverified
     */
    std::string issuer_name;
    
    /**
     * Message reception timestamp (local system time)
     */
    uint64_t received_at_ms = 0;
    
    /**
     * Decoded payload (only one will be populated based on frame_type)
     * Uses std::variant to safely hold different message types
     */
    std::variant<BasicSafetyMessage, SignalPhaseAndTiming, PersonalSafetyMessage> payload;
    
    /**
     * Get human-readable frame type name
     */
    std::string frame_type_name() const {
        switch (frame_type) {
            case MessageFrameType::BSM:     return "BSM (Basic Safety Message)";
            case MessageFrameType::SPAT:    return "SPaT (Signal Phase & Timing)";
            case MessageFrameType::PSM:     return "PSM (Personal Safety Message)";
            case MessageFrameType::TIM:     return "TIM (Traveler Info)";
            case MessageFrameType::EVA:     return "EVA (Emergency Vehicle Alert)";
            case MessageFrameType::TCM:     return "TCM (Traffic Control)";
            default:                        return "UNKNOWN";
        }
    }
    
    /**
     * Helper: Get BSM from variant (if frame_type == BSM)
     * @throws std::bad_variant_access if variant doesn't hold BSM
     */
    BasicSafetyMessage& get_bsm() {
        return std::get<BasicSafetyMessage>(payload);
    }
    
    const BasicSafetyMessage& get_bsm() const {
        return std::get<BasicSafetyMessage>(payload);
    }
    
    /**
     * Helper: Get SPaT from variant (if frame_type == SPAT)
     * @throws std::bad_variant_access if variant doesn't hold SPaT
     */
    SignalPhaseAndTiming& get_spat() {
        return std::get<SignalPhaseAndTiming>(payload);
    }
    
    const SignalPhaseAndTiming& get_spat() const {
        return std::get<SignalPhaseAndTiming>(payload);
    }
    
    /**
     * Helper: Get PSM from variant (if frame_type == PSM)
     * @throws std::bad_variant_access if variant doesn't hold PSM
     */
    PersonalSafetyMessage& get_psm() {
        return std::get<PersonalSafetyMessage>(payload);
    }
    
    const PersonalSafetyMessage& get_psm() const {
        return std::get<PersonalSafetyMessage>(payload);
    }
};

}  // namespace sentinel::v2x
