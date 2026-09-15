#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace Tavoos {

template<typename T>
struct EnableBitmaskOperators : std::false_type {};

template<typename T>
concept BitmaskEnum = std::is_enum_v<T> && EnableBitmaskOperators<T>::value;

template<BitmaskEnum T>
constexpr T operator|(T lhs, T rhs) noexcept {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template<BitmaskEnum T>
constexpr T operator&(T lhs, T rhs) noexcept {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template<BitmaskEnum T>
constexpr T operator^(T lhs, T rhs) noexcept {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template<BitmaskEnum T>
constexpr T operator~(T value) noexcept {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<U>(value));
}

template<BitmaskEnum T>
constexpr T& operator|=(T& lhs, T rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

template<BitmaskEnum T>
constexpr T& operator&=(T& lhs, T rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

template<BitmaskEnum T>
constexpr T& operator^=(T& lhs, T rhs) noexcept {
    lhs = lhs ^ rhs;
    return lhs;
}

template<BitmaskEnum T>
constexpr bool hasFlag(T value, T flag) noexcept {
    using U = std::underlying_type_t<T>;
    return (static_cast<U>(value) & static_cast<U>(flag)) != U{0};
}

template<BitmaskEnum T>
constexpr T withoutFlag(T value, T flag) noexcept {
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(value) & ~static_cast<U>(flag));
}

enum class Alignment : std::uint32_t {
    None = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    Top = 1 << 2,
    Bottom = 1 << 3,
    CenterHorizontal = 1 << 4,
    CenterVertical = 1 << 5,
    Center = CenterHorizontal | CenterVertical,
};

template<>
struct EnableBitmaskOperators<Alignment> : std::true_type {};

inline constexpr Alignment HorizontalAlignment = Alignment::Left | Alignment::Right | Alignment::CenterHorizontal;

inline constexpr Alignment VerticalAlignment = Alignment::Top | Alignment::Bottom | Alignment::CenterVertical;

enum class Fill : std::uint32_t {
    None = 0,
    Width = 1 << 0,
    Height = 1 << 1,
    Both = Width | Height,
};

template<>
struct EnableBitmaskOperators<Fill> : std::true_type {};

enum class FlexDirection {
    Row,
    Column,
};

enum class FlexWrap {
    NoWrap,
    Wrap,
};

enum class FlexJustify {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

enum class FlexAlign {
    Start,
    Center,
    End,
    Stretch,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

struct Color {
    static constexpr Color rgba(int red, int green, int blue, int alpha = 255) noexcept {
        return Color {
            red / 255.0f,
                    green / 255.0f,
                    blue / 255.0f,
                    alpha / 255.0f
        };
    }

    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    static const Color White;
    static const Color Black;
    static const Color Transparent;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Gray;
    static const Color LightGray;
    static const Color DarkGray;
    static const Color Orange;
    static const Color Purple;
    static const Color Pink;
    static const Color Brown;
};

inline constexpr Color Color::White       = Color::rgba(255, 255, 255);
inline constexpr Color Color::Black       = Color::rgba(0, 0, 0);
inline constexpr Color Color::Transparent = Color::rgba(0, 0, 0, 0);
inline constexpr Color Color::Red         = Color::rgba(255, 0, 0);
inline constexpr Color Color::Green       = Color::rgba(0, 255, 0);
inline constexpr Color Color::Blue        = Color::rgba(0, 0, 255);
inline constexpr Color Color::Yellow      = Color::rgba(255, 255, 0);
inline constexpr Color Color::Cyan        = Color::rgba(0, 255, 255);
inline constexpr Color Color::Magenta     = Color::rgba(255, 0, 255);
inline constexpr Color Color::Gray        = Color::rgba(128, 128, 128);
inline constexpr Color Color::LightGray   = Color::rgba(211, 211, 211);
inline constexpr Color Color::DarkGray    = Color::rgba(64, 64, 64);
inline constexpr Color Color::Orange      = Color::rgba(255, 165, 0);
inline constexpr Color Color::Purple      = Color::rgba(128, 0, 128);
inline constexpr Color Color::Pink        = Color::rgba(255, 192, 203);
inline constexpr Color Color::Brown       = Color::rgba(139, 69, 19);

struct Paint {
    enum class Kind { Solid, LinearGradient, RadialGradient, ConicGradient };

    struct Stop {
        float position{0.0f};
        Color color{};
    };

    Kind kind{Kind::Solid};
    Color solid{Color::Black};
    std::vector<Stop> stops;
    float angle{0.0f};
    float centerX{0.5f}, centerY{0.5f};
    float radius{0.5f};

    Paint() = default;
    constexpr Paint(const Color& c) : solid{c} {}
};

inline Paint linearGradient(std::vector<Paint::Stop> stops, float angle = 0.0f) {
    Paint p;
    p.kind = Paint::Kind::LinearGradient;
    p.stops = std::move(stops);
    p.angle = angle;
    return p;
}

inline Paint radialGradient(std::vector<Paint::Stop> stops, float centerX = 0.5f, float centerY = 0.5f, float radius = 0.5f) {
    Paint p;
    p.kind = Paint::Kind::RadialGradient;
    p.stops = std::move(stops);
    p.centerX = centerX;
    p.centerY = centerY;
    p.radius = radius;
    return p;
}

inline Paint conicGradient(std::vector<Paint::Stop> stops, float centerX = 0.5f, float centerY = 0.5f, float startAngle = 0.0f) {
    Paint p;
    p.kind = Paint::Kind::ConicGradient;
    p.stops = std::move(stops);
    p.centerX = centerX;
    p.centerY = centerY;
    p.angle = startAngle;
    return p;
}

constexpr float lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

constexpr Color lerp(const Color& a, const Color& b, float t) noexcept {
    return Color{
        lerp(a.r, b.r, t),
                lerp(a.g, b.g, t),
                lerp(a.b, b.b, t),
                lerp(a.a, b.a, t),
    };
}

inline Paint lerp(const Paint& a, const Paint& b, float t) {
    if (a.kind != b.kind || a.stops.size() != b.stops.size())
        return t < 1.0f ? a : b;

    Paint result = b;
    result.solid = lerp(a.solid, b.solid, t);
    result.angle = lerp(a.angle, b.angle, t);
    result.centerX = lerp(a.centerX, b.centerX, t);
    result.centerY = lerp(a.centerY, b.centerY, t);
    result.radius = lerp(a.radius, b.radius, t);
    result.stops.clear();
    for (std::size_t i = 0; i < a.stops.size(); ++i) {
        result.stops.push_back(Paint::Stop{
                                   lerp(a.stops[i].position, b.stops[i].position, t),
                                   lerp(a.stops[i].color, b.stops[i].color, t),
                               });
    }
    return result;
}

template<typename T>
concept Interpolatable = requires(const T& a, const T& b, float t) {
{ lerp(a, b, t) } -> std::convertible_to<T>;
        };

}