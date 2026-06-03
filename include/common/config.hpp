#pragma once

#include <filesystem>
#include <chrono>

//========================================

/// Config namespace (paths, algorithms, default values)
namespace config
{

//========================================

/// Network traffic encryption algorithm (should have 256-bit block size)
constexpr const char* cipher = "AES-256-CTR";

/// Player password hashing algorithm
constexpr const char* digest = "SHA256";

/// Hashing algorithm digest size
constexpr size_t digest_size = 32;

/// Score database path
const std::filesystem::path score_db_path = "score.db";

/// Default server port
constexpr uint16_t default_port = 1337;

/// Default server address (used as a default option when connecting to a server)
constexpr const char* default_address = "127.0.0.1";

/// Network performance test duration
const auto performance_test_duration = std::chrono::seconds(5);

//========================================

} // namespace config

//========================================