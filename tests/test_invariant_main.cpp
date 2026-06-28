#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

// Forward declaration of the function under test
extern "C" {
    uint32_t remove_03(uint8_t *bptr, uint32_t len);
}

class SecurityTest : public ::testing::TestWithParam<std::vector<uint8_t>> {};

TEST_P(SecurityTest, Remove03MaintainsMemorySafety) {
    // Invariant: remove_03 must never access memory outside the buffer bounds
    std::vector<uint8_t> payload = GetParam();
    std::vector<uint8_t> buffer = payload;
    
    // Call the actual production function
    uint32_t result_len = remove_03(buffer.data(), static_cast<uint32_t>(buffer.size()));
    
    // Security property: result length must not exceed original buffer size
    ASSERT_LE(result_len, buffer.size());
    
    // Additional safety: ensure no out-of-bounds writes occurred
    // (test passes if no crash or sanitizer violation occurs)
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    SecurityTest,
    ::testing::Values(
        // Exact exploit case: pattern causing miscalculated memmove length
        std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00},
        // Boundary case: minimal buffer with emulation byte at end
        std::vector<uint8_t>{0x00, 0x00, 0x03},
        // Valid input: normal NAL unit without issues
        std::vector<uint8_t>{0x00, 0x00, 0x01, 0xAB, 0xCD},
        // Edge case: buffer size causing integer underflow
        std::vector<uint8_t>{0x00, 0x00, 0x03, 0x03},
        // Large valid pattern to test loop bounds
        std::vector<uint8_t>(100, 0x00)
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}