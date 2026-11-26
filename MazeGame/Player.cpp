#include "Player.h"
#include "Maze.h"
#include <GL/freeglut.h>
#include <cmath>

static float Rad(float deg) { return deg * (float)M_PI / 180.0f; }

void Player::Forward(float& dx, float& dz) const
{
    dx = std::sin(Rad(yaw));
    dz = -std::cos(Rad(yaw));
}

void Player::Move(const Maze& maze, float dx, float dz)
{
    const float maxStep = radius * 0.5f;                   // 한 단계에 반지름의 절반까지만
    const float dist = std::sqrt(dx * dx + dz * dz);
    const int steps = (int)std::ceil(dist / maxStep);
    for (int i = 0; i < steps; ++i) {
        const float sx = dx / steps, sz = dz / steps;
        if (!maze.Collides(x + sx, z, radius)) x += sx;   // x축 먼저
        if (!maze.Collides(x, z + sz, radius)) z += sz;   // 그다음 z축
    }
}

void Player::ApplyCamera() const
{
    // 시선 벡터: yaw로 수평 방향, pitch로 위아래
    const float cp = std::cos(Rad(pitch));
    const float lx = std::sin(Rad(yaw)) * cp, ly = std::sin(Rad(pitch)), lz = -std::cos(Rad(yaw)) * cp;
    gluLookAt(x, eyeHeight, z, x + lx, eyeHeight + ly, z + lz, 0, 1, 0);
}
