/* danielsinkin97@gmail.com */
#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "constants.hpp"
#include "types.hpp"

namespace UTIL {
[[nodiscard]] inline auto window_normalized_to_ndc(
    const Position &norm_pos,
    float aspect_ratio) -> Position {
    auto pos = Position{
        norm_pos.x * 2.0f - 1.0f,
        1.0f - norm_pos.y * 2.0f};
    pos.x *= aspect_ratio;
    return pos;
}

[[nodiscard]] inline auto
ndc_to_window_normalized(const Position &ndc_pos, float aspect_ratio) -> Position {
    return Position{
        (ndc_pos.x / aspect_ratio + 1.0f) * 0.5f,
        (1.0f - ndc_pos.y) * 0.5f};
}

[[nodiscard]] inline auto
format_time(std::chrono::system_clock::time_point tp) -> std::string {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm *tm_ptr = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

[[nodiscard]] inline auto
format_duration(std::chrono::duration<float> duration) -> std::string {
    auto total_ms = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());

    auto hrs = total_ms / (60LL * 60LL * 1000LL);
    total_ms %= (60LL * 60LL * 1000LL);
    auto mins = total_ms / (60LL * 1000LL);
    total_ms %= (60LL * 1000LL);
    auto secs = total_ms / 1000LL;
    auto millis = total_ms % 1000LL;

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer),
        "%02lld:%02lld:%02lld.%03lld",
        hrs, mins, secs, millis);
    return std::string(buffer);
}

constexpr double byte_to_kb(std::size_t bytes) {
    return bytes / 1024.0;
}

constexpr double byte_to_mb(std::size_t bytes) {
    return bytes / (1024.0 * 1024.0);
}

constexpr double byte_to_gb(std::size_t bytes) {
    return bytes / (1024.0 * 1024.0 * 1024.0);
}
} // namespace UTIL