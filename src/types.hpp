/* danielsinkin97@gmail.com */
#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using glm::vec2;
using glm::vec3;
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace TYPES {
using Byte = uint8_t;
using Word = uint16_t;

struct Position {
    float x;
    float y;
};

[[nodiscard]] inline auto to_string(const Position &pos) -> std::string {
    return "(" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")";
}

// Updates global.input.mouse_pos in normalized window coordinates
inline std::ostream &operator<<(std::ostream &os, const Position &p) {
    return os
           << "Position("
           << p.x << ", "
           << p.y
           << ")";
}

[[nodiscard]] auto position_to_vec2(const Position &pos) -> vec2 {
    return vec2{pos.x, pos.y};
}
[[nodiscard]] auto vec2_to_position(const vec2 &vec) -> Position {
    return Position{vec.x, vec.y};
}
[[nodiscard]] auto distance(const Position &p1, const Position &p2) -> float {
    return glm::distance(position_to_vec2(p1), position_to_vec2(p2));
}

struct Color {
    float r, g, b;
};

[[nodiscard]] auto color_to_vec3(const Color &color) -> vec3 {
    return vec3{color.r, color.g, color.b};
}
[[nodiscard]] auto vec3_to_color(const vec3 &vec) -> Color {
    return Color{vec.x, vec.y, vec.z};
}
[[nodiscard]] auto color_from_u8(uint8_t r, uint8_t g, uint8_t b) -> Color {
    return Color{r / 255.0f, g / 255.0f, b / 255.0f};
}
[[nodiscard]] auto color_from_u8(const std::array<uint8_t, 3> &rgb) -> Color {
    return Color{rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f};
}
[[nodiscard]] auto color_multiply(const Color c1, const Color c2) -> Color {
    return Color{c1.r * c2.r, c1.g * c2.g, c1.b * c2.b};
}
[[nodiscard]] auto color_mix(const Color &c1, const Color &c2, float t) -> Color {
    return {
        c1.r * (1.0f - t) + c2.r * t,
        c1.g * (1.0f - t) + c2.g * t,
        c1.b * (1.0f - t) + c2.b * t};
}

inline std::ostream &operator<<(std::ostream &os, const Color &c) {
    return os
           << "Color("
           << c.r << ", "
           << c.g << ", "
           << c.b
           << ")";
}
struct Rect {
    Position position;
    float width;
    float height;
};

[[nodiscard]] auto rect_point_inside(const Rect &rect, const Position &pos) -> bool {
    return pos.x >= rect.position.x &&
           pos.x <= rect.position.x + rect.width &&
           pos.y >= rect.position.y - rect.height &&
           pos.y <= rect.position.y;
}
[[nodiscard]] auto get_center_position(const Rect &rect) -> Position {
    return Position{rect.position.x + rect.width / 2.0f, rect.position.y - rect.height / 2.0f};
}

[[nodiscard]] auto check_collision(const Rect &r1, const Rect &r2) -> bool {
    bool xcoll = r1.position.x < r2.position.x + r2.width &&
                 r1.position.x + r1.width > r2.position.x;

    bool ycoll = r1.position.y > r2.position.y - r2.height &&
                 r1.position.y - r1.height < r2.position.y;

    return xcoll && ycoll;
}

enum class CollisionDirection {
    None,
    Left,
    Right,
    Top,
    Bottom
};

[[nodiscard]] auto check_collision_directional(const Rect &b1, const Rect &b2) -> CollisionDirection {
    float left1 = b1.position.x;
    float right1 = b1.position.x + b1.width;
    float top1 = b1.position.y;
    float bottom1 = b1.position.y - b1.height;

    float left2 = b2.position.x;
    float right2 = b2.position.x + b2.width;
    float top2 = b2.position.y;
    float bottom2 = b2.position.y - b2.height;

    bool xcoll = (left1 < right2) &&
                 (right1 > left2);
    bool ycoll = (top1 > bottom2) &&
                 (bottom1 < top2);
    if (!(xcoll && ycoll)) {
        return CollisionDirection::None;
    }

    float c1x = (left1 + right1) * 0.5f;
    float c1y = (top1 + bottom1) * 0.5f;
    float c2x = (left2 + right2) * 0.5f;
    float c2y = (top2 + bottom2) * 0.5f;

    float dx = c2x - c1x;
    float dy = c2y - c1y;

    float penX = (b1.width * 0.5f + b2.width * 0.5f) - std::abs(dx);
    float penY = (b1.height * 0.5f + b2.height * 0.5f) - std::abs(dy);

    if (penX < penY) {
        return (dx > 0) ? CollisionDirection::Left : CollisionDirection::Right;
    } else {
        return (dy > 0) ? CollisionDirection::Bottom : CollisionDirection::Top;
    }
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Position, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rect, position, width, height)
} // namespace TYPES