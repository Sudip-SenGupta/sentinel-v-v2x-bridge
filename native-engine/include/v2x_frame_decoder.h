#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>

#include "v2x_structures.hpp"

namespace sentinel::v2x {

class V2XFrameDecodeException : public std::runtime_error {
public:
    explicit V2XFrameDecodeException(const std::string& message)
        : std::runtime_error(message) {}
};

class V2XFrameBufferException : public V2XFrameDecodeException {
public:
    explicit V2XFrameBufferException(const std::string& message)
        : V2XFrameDecodeException(message) {}
};

class V2XFrameDecoder {
public:
    V2XFrameDecoder() = delete;

    static MessageFrameType detect_frame_type(const std::vector<uint8_t>& payload);

    static DecodedV2XMessage decode(
        const std::vector<uint8_t>& payload,
        MessageFrameType frame_type
    );

    static std::string frame_type_to_string(MessageFrameType frame_type);
};

} // namespace sentinel::v2x