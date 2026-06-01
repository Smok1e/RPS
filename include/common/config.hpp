#pragma once

#include <filesystem>
#include <chrono>

//========================================

namespace config
{

//========================================

constexpr const char* cipher = "aes-256-ctr";
constexpr const char* digest = "SHA256";
constexpr size_t digest_size = 32;
const std::filesystem::path score_db_path = "score.db";

constexpr uint16_t default_port = 1337;
constexpr const char* default_address = "127.0.0.1";

const auto performance_test_duration = std::chrono::seconds(5);

//========================================

} // namespace config

//========================================