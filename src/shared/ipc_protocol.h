#pragma once

#include <cstdint>

namespace suyan {
namespace ipc {

constexpr uint32_t PROTOCOL_VERSION = 1;

constexpr wchar_t PIPE_NAME[] = L"\\\\.\\pipe\\SuYanInputMethod";

enum class Command : uint32_t {
    Handshake    = 0x0001,
    Disconnect   = 0x0002,
    TestKey      = 0x0101,
    ProcessKey   = 0x0102,
    FocusIn      = 0x0201,
    FocusOut     = 0x0202,
    UpdateCursor = 0x0203,
    ToggleMode   = 0x0301,
    ToggleLayout = 0x0302,
    QueryMode    = 0x0303,
};

enum Modifier : uint32_t {
    ModNone    = 0x00,
    ModShift   = 0x01,
    ModControl = 0x02,
    ModAlt     = 0x04,
};

#pragma pack(push, 1)
struct Request {
    Command cmd;
    uint32_t sessionId;
    uint32_t param1;
    uint32_t param2;
};
static_assert(sizeof(Request) == 16, "Request must be 16 bytes");

struct ResponseHeader {
    uint32_t result;
    uint32_t dataSize;
};
static_assert(sizeof(ResponseHeader) == 8, "ResponseHeader must be 8 bytes");
#pragma pack(pop)

struct CursorPosition {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;

    static void encode(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint32_t& param1, uint32_t& param2) {
        param1 = (uint32_t(uint16_t(x)) << 16) | uint16_t(y);
        param2 = (uint32_t(uint16_t(w)) << 16) | uint16_t(h);
    }

    static void decode(uint32_t param1, uint32_t param2,
                       int16_t& x, int16_t& y, int16_t& w, int16_t& h) {
        x = int16_t(param1 >> 16);
        y = int16_t(param1 & 0xFFFF);
        w = int16_t(param2 >> 16);
        h = int16_t(param2 & 0xFFFF);
    }
};

inline void serializeRequest(const Request& req, uint8_t* buffer) {
    auto* p = reinterpret_cast<const uint8_t*>(&req);
    for (size_t i = 0; i < sizeof(Request); ++i) {
        buffer[i] = p[i];
    }
}

inline Request deserializeRequest(const uint8_t* buffer) {
    Request req;
    auto* p = reinterpret_cast<uint8_t*>(&req);
    for (size_t i = 0; i < sizeof(Request); ++i) {
        p[i] = buffer[i];
    }
    return req;
}

inline void serializeResponseHeader(const ResponseHeader& hdr, uint8_t* buffer) {
    auto* p = reinterpret_cast<const uint8_t*>(&hdr);
    for (size_t i = 0; i < sizeof(ResponseHeader); ++i) {
        buffer[i] = p[i];
    }
}

inline ResponseHeader deserializeResponseHeader(const uint8_t* buffer) {
    ResponseHeader hdr;
    auto* p = reinterpret_cast<uint8_t*>(&hdr);
    for (size_t i = 0; i < sizeof(ResponseHeader); ++i) {
        p[i] = buffer[i];
    }
    return hdr;
}

}  // namespace ipc
}  // namespace suyan
