#pragma once
// 게임 진행: 시작 화면 → 미로 탐색(아이템 줍기, 미노타우르스에게 전달, 능력 해금) → 출구로 탈출
#include <string>
#include "Ability.h"
#include "ItemModel.h"
#include "Maze.h"
#include "Mesh.h"
#include "Player.h"

class Game {
public:
    bool Init();                         // 창을 만든 뒤 한 번: 지도·아이템 모델 불러오기
    void Update(float dt);
    void Draw(int w, int h);

    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void SpecialDown(int key);
    void SpecialUp(int key);
    void MouseMove(int x, int y, int w, int h);
    bool WantsQuit() const { return quit; }

    // ── 능력 코드(Ability)가 쓰는 정보 ──
    bool ItemPosition(char item, float& x, float& z) const;  // 지도에 놓인 위치 (칸 중심)
    bool IsCarried(char item) const;                         // 들고 있는가
    bool IsDelivered(char item) const;                       // 미노타우르스에게 전달했는가 (= 능력 해금)
    void ShowMessage(const std::string& text, float seconds = 3.0f);

    Player player;
    Maze   maze;
    float  viewDistance = 7.0f;          // 가시거리 (등불 능력이 늘린다)
    bool   xray = false;                 // 투시 (아르고스의 눈 능력이 켠다)

private:
    enum class State { Title, Play, Clear };
    struct Item {
        char letter; const char* name; const char* file; float height; float color[3];
        ItemModel model;
        float x = 0, z = 0;
        bool onMap = false, carried = false, delivered = false;
    };

    void Reset();
    void Interact();                     // E 키
    World MakeWorld(float dt);
    void DrawItem(const Item& it, bool transparent) const;
    void DrawMinotaur() const;
    void DrawPedestal(float x, float z) const;
    void PushOutOfPedestals();           // 단상과 겹치면 밖으로 밀어낸다
    void DrawHud(int w, int h);

    State state = State::Title;
    Item  items[4] = {
        { 'L', "Lamp",             "Models/lamp.item",    0.60f, { 1.0f, 0.75f, 0.35f } },
        { 'H', "Hermes' Sandals",  "Models/hermes.item",  0.40f, { 0.95f, 0.85f, 0.55f } },
        { 'A', "Eye of Argus",     "Models/argos.item",   0.50f, { 0.45f, 0.75f, 1.0f } },
        { 'T', "Ariadne's Thread", "Models/ariadne.item", 0.50f, { 0.95f, 0.35f, 0.35f } },
    };
    float minoX = 0, minoZ = 0, barsX = 0, barsZ = 0;   // 미노타우르스와 창살 위치
    bool  hasMinotaur = false, metMinotaur = false;
    float safeX = 0, safeZ = 0;          // 마지막으로 안전했던 위치 (구덩이에 떨어지면 여기로)
    bool  NearBars() const;
    void  KeepSafe();
    Mesh  pedestal;                      // 아이템 단상 (Models/pedestal.dat, 분할 4인 SOR)
    float time = 0, lastDt = 0;
    std::string message; float messageTime = 0;
    bool keys[256] = {}, special[512] = {};
    bool mouseCentered = false;
    bool quit = false;
};
