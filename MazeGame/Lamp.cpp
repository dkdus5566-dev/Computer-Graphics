// 등불 능력: 가시거리 확보
// 기본 버전 (미로 기반 담당이 동작 확인용으로 작성). 등불 담당이 자유롭게 고치고 늘린다.
//   예: F 키로 켜고 끄기, 불빛 색·깜빡임, 기름이 줄어드는 기능
#include <GL/freeglut.h>
#include "Ability.h"
#include "Game.h"

namespace {
const float kLampViewDistance = 14.0f;   // 기본 7 → 14 (안개가 끝나는 거리, 불빛 감쇠도 이 값을 따른다)

void OnUnlock(World& w)
{
    w.viewDistance = kLampViewDistance;
    w.game.ShowMessage("The Minotaur mends the Lamp.\nIts light pushes back the darkness of the labyrinth.", 5.0f);
}

bool registered = RegisterAbility({ 'L', OnUnlock });
}
