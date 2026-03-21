/**
 * @file v2x_message_frame.cpp
 * @brief V2X Message Frame Type Detection and Decoding
 * 
 * Phase 4 Delivery: Decode IEEE 1609.2 message frames into structured data
 * 
 * Converts raw COER-encoded payloads into:
 *   - BSM (Basic Safety Message) - vehicle telemetry
 *   - SPaT (Signal Phase and Timing) - traffic signals
 *   - PSM (Personal Safety Message) - pedestrians/cyclists
 *   - Other IEEE 1609.2 frame types
 * 
 * @author Sentinel V2X Bridge
 * @date March 11, 2026
 * @version 1.0.0
 */

#include "v2x_frame_decoder.h"
#include "v2x_structures.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace sentinel::v2x {

// ============================================================================
// Message Frame Type Detection
// ============================================================================

MessageFrameType V2XFrameDecoder::detect_frame_type(const std::vector<uint8_t>& payload) {
    /**
     * IEEE 1609.2 Frame Format:
     * 
     * Byte 0: Frame Type Identifier (high bits) + Flags (low bits)
     * 
     * Frame Type IDs (IEEE 1609.2-2016 Section 4):
     *   0x01 = BSM (Basic Safety Message)
     *   0x02 = SPaT (Signal Phase and Timing)
     *   0x03 = PSM (Personal Safety Message)
     *   0x04 = TIM (Traveler Information Message)
     *   0x05 = EVA (Emergency Vehicle Alert)
     *   0x06 = TCM (Traffic Control Message)
     * 
     * The frame type is encoded in the first byte of the payload after
     * the COER header has been parsed and removed.
     */
    
    if (payload.empty()) {
        throw V2XFrameBufferException("Empty payload: cannot determine frame type");
    }
    
    if (payload.size() < 3) {
        throw V2XFrameBufferException(
            "Payload too short (" + std::to_string(payload.size()) + 
            " bytes, need at least 3 for frame type detection)"
        );
    }
    
    // Extract frame type from first byte
    uint8_t frame_byte = payload[0];
    uint8_t frame_type_id = (frame_byte >> 4) & 0x0F;  // Upper 4 bits
    
    // Map to enum
    switch (frame_type_id) {
        case 0x01: return MessageFrameType::BSM;
        case 0x02: return MessageFrameType::SPAT;
        case 0x03: return MessageFrameType::PSM;
        case 0x04: return MessageFrameType::TIM;
        case 0x05: return MessageFrameType::EVA;
        case 0x06: return MessageFrameType::TCM;
        default:   return MessageFrameType::UNKNOWN;
    }
}

// ============================================================================
// Message Frame Decoding (Payload Extraction)
// ============================================================================

/**
 * Helper: Extract latitude/longitude from COER encoding
 * 
 * IEEE 1609.2 uses 32-bit signed values in units of 1/10,000,000 degree
 */
static double extract_coordinate(const std::vector<uint8_t>& data, size_t& pos) {
    if (pos + 4 > data.size()) {
        throw V2XFrameBufferException("Cannot read coordinate: truncated buffer");
    }
    
    int32_t value = 0;
    value |= (static_cast<int32_t>(data[pos]) << 24);
    value |= (static_cast<int32_t>(data[pos + 1]) << 16);
    value |= (static_cast<int32_t>(data[pos + 2]) << 8);
    value |= static_cast<int32_t>(data[pos + 3]);
    pos += 4;
    
    // Convert from 1/10,000,000 degree to degrees
    return static_cast<double>(value) / 10000000.0;
}

/**
 * Decode BSM (Basic Safety Message)
 * 
 * BSM Format (simplified IEEE 1609.2):
 * - Frame type (1 byte)
 * - Timestamp (4 bytes, milliseconds)
 * - Sender ID (6 bytes)
 * - Sequence number (1 byte)
 * - Latitude (4 bytes, signed 32-bit, units of 1/10M degree)
 * - Longitude (4 bytes)
 * - Speed (2 bytes, unsigned 16-bit, units of 0.02 m/s)
 * - Heading (2 bytes, unsigned 16-bit, 0-35900 in 0.01 degree units)
 * - Acceleration (2 bytes, signed, units of 0.01 m/s²)
 */
static BasicSafetyMessage decode_bsm(const std::vector<uint8_t>& payload) {
    BasicSafetyMessage bsm;
    size_t pos = 1;  // Skip frame type byte
    
    try {
        // Timestamp (milliseconds)
        if (pos + 4 <= payload.size()) {
            uint32_t ts = 0;
            ts |= (static_cast<uint32_t>(payload[pos]) << 24);
            ts |= (static_cast<uint32_t>(payload[pos + 1]) << 16);
            ts |= (static_cast<uint32_t>(payload[pos + 2]) << 8);
            ts |= static_cast<uint32_t>(payload[pos + 3]);
            bsm.timestamp_ms = ts;
            pos += 4;
        }
        
        // Sender ID (MAC address - 6 bytes)
        if (pos + 6 <= payload.size()) {
            constexpr int buf_size = 18;  // "XX:XX:XX:XX:XX:XX"
            char mac_str[buf_size] = {0};
            snprintf(mac_str, buf_size, "%02X:%02X:%02X:%02X:%02X:%02X",
                    payload[pos], payload[pos+1], payload[pos+2],
                    payload[pos+3], payload[pos+4], payload[pos+5]);
            bsm.sender_id = mac_str;
            pos += 6;
        }
        
        // Sequence number
        if (pos < payload.size()) {
            bsm.sequence_num = payload[pos++];
        }
        
        // Latitude
        if (pos + 4 <= payload.size()) {
            bsm.position.latitude = extract_coordinate(payload, pos);
        }
        
        // Longitude
        if (pos + 4 <= payload.size()) {
            bsm.position.longitude = extract_coordinate(payload, pos);
        }
        
        // Speed (0.02 m/s per unit)
        if (pos + 2 <= payload.size()) {
            uint16_t speed_raw = (static_cast<uint16_t>(payload[pos]) << 8) |
                                 payload[pos + 1];
            bsm.motion.speed = speed_raw * 0.02f;
            pos += 2;
        }
        
        // Heading (0.01 degree per unit, 0-35900)
        if (pos + 2 <= payload.size()) {
            uint16_t heading_raw = (static_cast<uint16_t>(payload[pos]) << 8) |
                                   payload[pos + 1];
            bsm.motion.heading = (heading_raw * 0.01f);
            if (bsm.motion.heading > 359.9f) {
                bsm.motion.heading = 0.0f;  // Invalid/unknown
            }
            pos += 2;
        }
        
        // Acceleration (0.01 m/s² per unit, signed)
        if (pos + 2 <= payload.size()) {
            int16_t accel_raw = (static_cast<int16_t>(payload[pos]) << 8) |
                                payload[pos + 1];
            bsm.motion.acceleration = accel_raw * 0.01f;
            pos += 2;
        }
        
    } catch (const std::exception& e) {
        throw V2XFrameDecodeException("BSM decoding failed: " + std::string(e.what()));
    }
    
    return bsm;
}

/**
 * Decode SPaT (Signal Phase and Timing)
 * 
 * SPaT Format (simplified):
 * - Frame type (1 byte)
 * - Timestamp (4 bytes)
 * - Intersection count (1 byte)
 * - For each intersection:
 *   - Intersection ID (4 bytes)
 *   - Current phase (1 byte)
 *   - Time in phase (1 byte)
 *   - Next phase (1 byte)
 *   - Time to next (1 byte)
 */
static SignalPhaseAndTiming decode_spat(const std::vector<uint8_t>& payload) {
    SignalPhaseAndTiming spat;
    size_t pos = 1;  // Skip frame type
    
    try {
        // Timestamp
        if (pos + 4 <= payload.size()) {
            uint32_t ts = 0;
            ts |= (static_cast<uint32_t>(payload[pos]) << 24);
            ts |= (static_cast<uint32_t>(payload[pos + 1]) << 16);
            ts |= (static_cast<uint32_t>(payload[pos + 2]) << 8);
            ts |= static_cast<uint32_t>(payload[pos + 3]);
            spat.timestamp_ms = ts;
            pos += 4;
        }
        
        // Intersection count
        uint8_t intersection_count = 0;
        if (pos < payload.size()) {
            intersection_count = payload[pos++];
        }
        
        // Parse each intersection state
        for (uint8_t i = 0; i < intersection_count && pos < payload.size(); ++i) {
            IntersectionState state;
            
            // Intersection ID
            if (pos + 4 <= payload.size()) {
                state.intersection_id = 
                    (static_cast<uint32_t>(payload[pos]) << 24) |
                    (static_cast<uint32_t>(payload[pos + 1]) << 16) |
                    (static_cast<uint32_t>(payload[pos + 2]) << 8) |
                    static_cast<uint32_t>(payload[pos + 3]);
                pos += 4;
            }
            
            // Phase info (4 bytes)
            if (pos + 4 <= payload.size()) {
                state.current_phase = payload[pos];
                state.time_in_phase_sec = payload[pos + 1];
                state.next_phase = payload[pos + 2];
                state.time_to_next_phase_sec = payload[pos + 3];
                pos += 4;
            }
            
            spat.intersections.push_back(state);
        }
        
    } catch (const std::exception& e) {
        throw V2XFrameDecodeException("SPaT decoding failed: " + std::string(e.what()));
    }
    
    return spat;
}

/**
 * Decode PSM (Personal Safety Message)
 */
static PersonalSafetyMessage decode_psm(const std::vector<uint8_t>& payload) {
    PersonalSafetyMessage psm;
    size_t pos = 1;  // Skip frame type
    
    try {
        // Timestamp
        if (pos + 4 <= payload.size()) {
            uint32_t ts = 0;
            ts |= (static_cast<uint32_t>(payload[pos]) << 24);
            ts |= (static_cast<uint32_t>(payload[pos + 1]) << 16);
            ts |= (static_cast<uint32_t>(payload[pos + 2]) << 8);
            ts |= static_cast<uint32_t>(payload[pos + 3]);
            psm.timestamp_ms = ts;
            pos += 4;
        }
        
        // User type and sender ID
        if (pos + 7 <= payload.size()) {
            psm.user_type = payload[pos];
            constexpr int buf_size = 18;
            char mac_str[buf_size] = {0};
            snprintf(mac_str, buf_size, "%02X:%02X:%02X:%02X:%02X:%02X",
                    payload[pos+1], payload[pos+2], payload[pos+3],
                    payload[pos+4], payload[pos+5], payload[pos+6]);
            psm.sender_id = mac_str;
            pos += 7;
        }
        
        // Position
        if (pos + 8 <= payload.size()) {
            psm.position.latitude = extract_coordinate(payload, pos);
            psm.position.longitude = extract_coordinate(payload, pos);
        }
        
        // Heading and speed
        if (pos + 4 <= payload.size()) {
            uint16_t heading_raw = (static_cast<uint16_t>(payload[pos]) << 8) |
                                   payload[pos + 1];
            psm.heading = heading_raw * 0.01f;
            
            uint16_t speed_raw = (static_cast<uint16_t>(payload[pos + 2]) << 8) |
                                 payload[pos + 3];
            psm.speed = speed_raw * 0.02f;
            pos += 4;
        }
        
    } catch (const std::exception& e) {
        throw V2XFrameDecodeException("PSM decoding failed: " + std::string(e.what()));
    }
    
    return psm;
}

// ============================================================================
// Main Frame Decoding Function
// ============================================================================

DecodedV2XMessage V2XFrameDecoder::decode(
    const std::vector<uint8_t>& payload,
    MessageFrameType frame_type
) {
    DecodedV2XMessage msg;
    msg.frame_type = frame_type;
    msg.received_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    try {
        switch (frame_type) {
            case MessageFrameType::BSM: {
                msg.payload = decode_bsm(payload);
                break;
            }
            case MessageFrameType::SPAT: {
                msg.payload = decode_spat(payload);
                break;
            }
            case MessageFrameType::PSM: {
                msg.payload = decode_psm(payload);
                break;
            }
            default: {
                msg.frame_type = MessageFrameType::UNKNOWN;
                throw V2XFrameDecodeException("Unsupported frame type");
            }
        }
    } catch (const V2XFrameDecodeException& e) {
        msg.frame_type = MessageFrameType::UNKNOWN;
        throw;
    }
    
    return msg;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string V2XFrameDecoder::frame_type_to_string(MessageFrameType frame_type) {
    switch (frame_type) {
        case MessageFrameType::BSM:     return "BSM (Basic Safety Message)";
        case MessageFrameType::SPAT:    return "SPaT (Signal Phase & Timing)";
        case MessageFrameType::PSM:     return "PSM (Personal Safety Message)";
        case MessageFrameType::TIM:     return "TIM (Traveler Information)";
        case MessageFrameType::EVA:     return "EVA (Emergency Vehicle Alert)";
        case MessageFrameType::TCM:     return "TCM (Traffic Control)";
        case MessageFrameType::UNKNOWN: return "UNKNOWN";
        default:                        return "UNRECOGNIZED";
    }
}

}  // namespace sentinel::v2x
