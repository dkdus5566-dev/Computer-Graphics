#pragma once
// 2D/3D 벡터와 기본 연산 (모델러·게임 공용)
#include <cmath>

// 2D 점 — SOR 프로파일에서 x = 반지름(회전축과의 거리), y = 높이
struct Vec2 {
    float x = 0.0f, y = 0.0f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

// 3D 벡터 — 정점 위치, 법선(normal)
struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
};

inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// 외적: a × b — 두 변에 수직인 벡터 (삼각형 면의 법선 방향)
inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline float Length(const Vec3& v) { return std::sqrt(Dot(v, v)); }

// 단위 벡터로 만들기 (길이 0이면 위쪽 방향을 돌려줌)
inline Vec3 Normalize(const Vec3& v) {
    float len = Length(v);
    return len > 1e-8f ? v * (1.0f / len) : Vec3(0.0f, 1.0f, 0.0f);
}
