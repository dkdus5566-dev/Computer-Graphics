// 아르고스의 눈 능력: Q를 누르면 일정 시간 투시 (가짜 벽이 보이고 지나갈 수 있으며, 남은 아이템이 벽 너머로 빛난다)
// 기본 버전 (미로 기반 담당이 동작 확인용으로 작성). 아르고스 담당이 자유롭게 고치고 늘린다.
//   투시 효과 자체(가짜 벽 표시·통과, 아이템 발광)는 미로 기반에 있다. 여기서는 w.xray를 켜고 끄기만 한다.
#include <GL/freeglut.h>
#include <algorithm>
#include "Ability.h"
#include "Game.h"

namespace {
const float kDuration = 5.0f;    // 투시 지속 시간 (초)
const float kCooldown = 8.0f;    // 다시 쓸 수 있을 때까지 (초, 사용 시작부터)

float active = 0, cooldown = 0;

void OnUnlock(World& w)
{
    active = cooldown = 0;
    w.game.ShowMessage("The Minotaur mends the Eye of Argus.\nPress Q to see through the walls for a while.", 5.0f);
}

void OnKey(World& w, unsigned char key)
{
    if (key != 'q' || cooldown > 0) return;
    active = kDuration;
    cooldown = kCooldown;
    w.xray = true;
}

void Update(World& w)
{
    if (cooldown > 0) cooldown = std::max(0.0f, cooldown - w.dt);
    if (active > 0) {
        active = std::max(0.0f, active - w.dt);
        w.xray = active > 0;
    }
}

// 화면 오른쪽 아래, 대쉬 게이지 위: 투시 남은 시간 / 충전
void DrawHud(World&, int w, int)
{
    const float x = w - 220.0f, y = 70.0f, width = 180.0f, height = 12.0f;
    const bool on = active > 0;
    const float fill = on ? active / kDuration : 1.0f - cooldown / kCooldown;
    glColor3f(0.12f, 0.18f, 0.25f);
    glRectf(x, y, x + width, y + height);
    if (on) glColor3f(0.5f, 0.8f, 1.0f); else if (fill >= 1.0f) glColor3f(0.4f, 0.7f, 1.0f); else glColor3f(0.3f, 0.45f, 0.6f);
    glRectf(x, y, x + width * fill, y + height);
    glRasterPos2f(x, y + 18);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)(on ? "SIGHT OF ARGUS" : fill >= 1.0f ? "SIGHT [Q] ready" : "SIGHT recharging"));
}

bool registered = RegisterAbility({ 'A', OnUnlock, Update, OnKey, nullptr, nullptr, DrawHud });
}
