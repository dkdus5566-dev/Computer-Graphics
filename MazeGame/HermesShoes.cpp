// 헤르메스의 신발 능력: 기본 이동속도 증가 + Shift 대쉬 (가시 구덩이를 넘는다)
// 사양: docs/items/hermes-shoes.md 4절
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include "Ability.h"
#include "Game.h"

namespace {
const float kSpeedBonus   = 1.25f;   // 신발을 받은 뒤 기본 이동속도 ×1.25
const float kDashTime     = 0.30f;   // 대쉬 지속 시간 (초)
const float kDashSpeed    = 4.0f;    // 대쉬 중 속도 = 기본 속도 × 4  → 약 5.1 이동 (구덩이 한 칸 2.0 + 여유)
const float kCooldown     = 1.5f;    // 다시 쓸 수 있을 때까지 (초)
const float kFovBoost     = 12.0f;   // 대쉬 중 시야각을 넓혀 속도감을 준다

float dashLeft = 0, cooldown = 0, baseFov = 70;
float dirX = 0, dirZ = -1;

void OnUnlock(World& w)
{
    w.player.speed *= kSpeedBonus;
    baseFov = w.player.fov;
    dashLeft = cooldown = 0;
    w.game.ShowMessage("The Minotaur mends Hermes' Sandals.\nYou move faster. Press Shift to dash over the spike pits.", 6.0f);
}

void OnSpecial(World& w, int key)
{
    if (key != GLUT_KEY_SHIFT_L && key != GLUT_KEY_SHIFT_R) return;
    if (dashLeft > 0 || cooldown > 0) return;
    w.player.Forward(dirX, dirZ);         // 바라보는 방향으로
    dashLeft = kDashTime;
    cooldown = kCooldown;
}

void Update(World& w)
{
    if (cooldown > 0) cooldown = std::max(0.0f, cooldown - w.dt);
    if (dashLeft > 0) {
        const float t = std::min(dashLeft, w.dt);
        dashLeft -= t;
        // 대쉬 중에만 가시 구덩이를 지나갈 수 있다. 빠르게 움직여도 Player::Move가 작은 단계로 나눠
        // 벽을 검사하므로 벽을 뚫지 않는다 (터널링 방지)
        w.maze.pitsPassable = true;
        const float d = w.player.speed * kDashSpeed * t;
        w.player.Move(w.maze, dirX * d, dirZ * d);
        w.maze.pitsPassable = dashLeft > 0;
    }
    // 시야각: 대쉬하면 넓어졌다가 부드럽게 돌아온다
    const float target = baseFov + (dashLeft > 0 ? kFovBoost : 0.0f);
    w.player.fov += (target - w.player.fov) * std::min(1.0f, w.dt * 12.0f);
}

// 화면 오른쪽 아래 대쉬 게이지 (차면 사용 가능)
void DrawHud(World&, int w, int)
{
    const float x = w - 220.0f, y = 30.0f, width = 180.0f, height = 12.0f;
    const float fill = 1.0f - cooldown / kCooldown;
    glColor3f(0.25f, 0.2f, 0.12f);
    glRectf(x, y, x + width, y + height);
    if (fill >= 1.0f) glColor3f(1.0f, 0.85f, 0.4f); else glColor3f(0.6f, 0.5f, 0.3f);
    glRectf(x, y, x + width * fill, y + height);
    glRasterPos2f(x, y + 18);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)(fill >= 1.0f ? "DASH [Shift] ready" : "DASH recharging"));
}

bool registered = RegisterAbility({ 'H', OnUnlock, Update, nullptr, OnSpecial, nullptr, DrawHud });
}
