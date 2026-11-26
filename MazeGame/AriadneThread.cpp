// 아리아드네의 실타래 능력: 탈출
// 전달하면 출구가 열리는 것은 미로 기반(Game.cpp)에 있다. 여기서는 T 키로 출구까지 바닥에 붉은 실을 보여 준다.
// 기본 버전 (미로 기반 담당이 동작 확인용으로 작성). 실타래 담당이 자유롭게 고치고 늘린다.
#include <GL/freeglut.h>
#include <cmath>
#include <queue>
#include <vector>
#include "Ability.h"
#include "Game.h"
#include "Maze.h"
#include "Player.h"

namespace {
bool showThread = true;

void OnUnlock(World& w)
{
    showThread = true;
    w.game.ShowMessage("The Minotaur mends Ariadne's Thread. The exit is open!\nFollow the red thread (T to hide or show).", 6.0f);
}

void OnKey(World&, unsigned char key)
{
    if (key == 't') showThread = !showThread;
}

// 플레이어가 있는 칸에서 출구 칸까지 너비 우선 탐색(BFS)으로 가장 짧은 칸 경로를 찾는다
bool FindPath(const Maze& m, int sc, int sr, int ec, int er, std::vector<int>& path)
{
    const int W = m.Cols(), H = m.Rows();
    std::vector<int> prev(W * H, -1);
    std::queue<int> q;
    q.push(sr * W + sc); prev[sr * W + sc] = sr * W + sc;
    const int dc[4] = { 1, -1, 0, 0 }, dr[4] = { 0, 0, 1, -1 };
    while (!q.empty()) {
        int cur = q.front(); q.pop();
        if (cur == er * W + ec) break;
        for (int k = 0; k < 4; ++k) {
            int c = cur % W + dc[k], r = cur / W + dr[k];
            if (c < 0 || r < 0 || c >= W || r >= H || prev[r * W + c] != -1) continue;
            const char ch = m.At(c, r);   // 가시 구덩이(대쉬)·가짜 벽(투시)은 지나갈 수 있는 길로 본다
            if (ch == '#' || ch == 'B' || ch == 'M') continue;
            prev[r * W + c] = cur; q.push(r * W + c);
        }
    }
    if (prev[er * W + ec] == -1) return false;
    path.clear();
    for (int cur = er * W + ec; ; cur = prev[cur]) { path.push_back(cur); if (prev[cur] == cur) break; }
    return true;
}

void DrawWorld(World& w)
{
    if (!showThread) return;
    int ec, er, pc, pr;
    if (!w.maze.Find('X', ec, er)) return;
    w.maze.CellOf(w.player.x, w.player.z, pc, pr);
    std::vector<int> path;
    if (!FindPath(w.maze, pc, pr, ec, er, path)) return;

    // 바닥 바로 위에 붉은 선. 조명을 끄고 살짝 빛나게 깜빡인다
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);
    const float pulse = 0.75f + 0.25f * std::sin(w.time * 3.0f);
    glColor3f(0.9f * pulse, 0.12f, 0.1f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < path.size(); ++i) {
        float x, z;
        w.maze.CellCenter(path[i] % w.maze.Cols(), path[i] / w.maze.Cols(), x, z);
        if (i + 1 == path.size()) { x = w.player.x; z = w.player.z; }   // 마지막 점 = 플레이어 발밑
        glVertex3f(x, 0.04f, z);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

bool registered = RegisterAbility({ 'T', OnUnlock, nullptr, OnKey, nullptr, DrawWorld });
}
