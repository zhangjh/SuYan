/**
 * IPC Protocol Property Tests
 * 
 * Feature: windows-platform-support
 * Property 1: IPC Message Round-Trip
 * Validates: Requirements 3.6, 3.7
 */

#include "../../src/shared/ipc_protocol.h"
#include <iostream>
#include <random>
#include <cstring>

using namespace suyan::ipc;

static std::mt19937 rng(42);

static uint32_t randomU32() {
    return std::uniform_int_distribution<uint32_t>()(rng);
}

static Command randomCommand() {
    Command cmds[] = {
        Command::Handshake, Command::Disconnect, Command::TestKey,
        Command::ProcessKey, Command::FocusIn, Command::FocusOut,
        Command::UpdateCursor
    };
    return cmds[std::uniform_int_distribution<size_t>(0, 6)(rng)];
}

static bool testRequestRoundTrip() {
    std::cout << "Property 1: Request Round-Trip... ";
    
    for (int i = 0; i < 100; ++i) {
        Request original;
        original.cmd = randomCommand();
        original.sessionId = randomU32();
        original.param1 = randomU32();
        original.param2 = randomU32();
        
        uint8_t buffer[16];
        serializeRequest(original, buffer);
        Request restored = deserializeRequest(buffer);
        
        if (original.cmd != restored.cmd ||
            original.sessionId != restored.sessionId ||
            original.param1 != restored.param1 ||
            original.param2 != restored.param2) {
            std::cout << "FAILED at iteration " << i << std::endl;
            std::cout << "  Original: cmd=" << static_cast<uint32_t>(original.cmd)
                      << " sessionId=" << original.sessionId
                      << " param1=" << original.param1
                      << " param2=" << original.param2 << std::endl;
            std::cout << "  Restored: cmd=" << static_cast<uint32_t>(restored.cmd)
                      << " sessionId=" << restored.sessionId
                      << " param1=" << restored.param1
                      << " param2=" << restored.param2 << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

static bool testResponseHeaderRoundTrip() {
    std::cout << "Property 1: ResponseHeader Round-Trip... ";
    
    for (int i = 0; i < 100; ++i) {
        ResponseHeader original;
        original.result = randomU32();
        original.dataSize = randomU32();
        
        uint8_t buffer[8];
        serializeResponseHeader(original, buffer);
        ResponseHeader restored = deserializeResponseHeader(buffer);
        
        if (original.result != restored.result ||
            original.dataSize != restored.dataSize) {
            std::cout << "FAILED at iteration " << i << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED (100 iterations)" << std::endl;
    return true;
}

int main() {
    std::cout << "=== IPC Protocol Property Tests ===" << std::endl;
    std::cout << "Feature: windows-platform-support" << std::endl;
    std::cout << "Validates: Requirements 3.6, 3.7" << std::endl;
    std::cout << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    if (testRequestRoundTrip()) ++passed; else ++failed;
    if (testResponseHeaderRoundTrip()) ++passed; else ++failed;
    
    std::cout << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
