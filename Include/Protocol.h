#pragma once

#include <cstdint>
#include <type_traits>
#include <chrono>

// Header layout (8 bytes, network byte order / big-endian):
// uint16_t signature (0xAA55)
// uint8_t  version (1)
// uint8_t  dataType (1=Subscribe (to server), 2=Data (to client), 3=Alive (to client))
// uint32_t len (payload length)

#pragma pack(push,1)
struct SSignalProtocolHeader
{
    uint16_t signature;
    uint8_t  version;
    uint8_t  dataType;
    uint32_t len;
};
#pragma pack(pop)
static_assert(sizeof(SSignalProtocolHeader) == 8, "Header must be 8 bytes");

const uint16_t SIGNAL_HEADER_SIGNATURE = 0xAA55;


// Signals

enum class ESignalType : uint8_t
{
    unknown = 0,
    discret = 1 << 0,
    analog = 1 << 1,
};

inline ESignalType operator|(ESignalType lhs, ESignalType rhs)
{
    using T = std::underlying_type_t<ESignalType>;
    return static_cast<ESignalType>(static_cast<T>(lhs) | static_cast<T>(rhs));
}


struct Signal
{
    using time_point = std::chrono::steady_clock::time_point;

    uint32_t id;
    ESignalType type;
    double value;
    time_point ts;

    Signal()
        : id(-1), type(ESignalType::unknown), value(0.0), ts(time_point())
    {
    }

    Signal(uint32_t _id, ESignalType _type, double _value = 0.0, time_point _ts = time_point())
        : id(_id), type(_type), value(_value), ts(_ts)
    {
    }
};