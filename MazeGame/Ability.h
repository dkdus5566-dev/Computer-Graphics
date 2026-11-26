#pragma once
// 아이템 능력을 게임에 붙이는 곳.
// 각 담당은 자기 파일(MazeGame/Lamp.cpp 등)에서 RegisterAbility를 한 번 호출하면 된다.
// 미로 기반 코드(Game.cpp)를 고칠 필요가 없다.
//
//   // MazeGame/Lamp.cpp 예시
//   #include "Ability.h"
//   static void OnUnlock(World& w) { w.viewDistance = 14.0f; }   // 가시거리 늘리기
//   static bool registered = RegisterAbility({ 'L', OnUnlock });
//
// 함수는 필요한 것만 채우고 나머지는 비워 둔다 (nullptr).
#include <vector>

class Player;
class Maze;
class Game;

// 능력 코드가 읽고 바꿀 수 있는 게임 상태
struct World {
    Game&   game;           // 아이템 위치 등 (Game.h의 공개 함수)
    Player& player;         // 위치, 방향, speedMultiplier, fov
    Maze&   maze;           // 벽 검사, exitOpen, pitsPassable(대쉬 중 가시 구덩이 통과)
    float&  viewDistance;   // 가시거리: 안개가 완전히 짙어지는 거리 (등불)
    bool&   xray;           // 투시: 켜져 있는 동안 가짜 벽이 보이고 지나갈 수 있으며, 아이템이 벽 너머로 빛난다 (아르고스의 눈)
    float   time;           // 게임 시작 후 지난 시간 (초)
    float   dt;             // 이번 프레임 시간 (초)
};

struct Ability {
    char item;                                          // 지도 글자: L H A T
    void (*onUnlock)(World&) = nullptr;                 // 미노타우르스에게 전달한 순간 한 번
    void (*update)(World&) = nullptr;                   // 해금된 뒤 매 프레임
    void (*onKey)(World&, unsigned char key) = nullptr; // 일반 키를 눌렀을 때 (해금된 뒤)
    void (*onSpecial)(World&, int key) = nullptr;       // 특수 키 (GLUT_KEY_SHIFT_L 등, 해금된 뒤)
    void (*drawWorld)(World&) = nullptr;                // 3D 장면에 추가로 그리기 (해금된 뒤)
    void (*drawHud)(World&, int w, int h) = nullptr;    // 화면 위 2D 표시 (좌표: 왼쪽 아래 0,0 ~ w,h)
};

bool RegisterAbility(const Ability& ability);
const std::vector<Ability>& RegisteredAbilities();
