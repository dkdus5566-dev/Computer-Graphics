#pragma once
// 글자 지도 파일(Maps/maze.txt)로 만든 사각형 미로: 불러오기, 그리기, 벽 충돌
//   #  벽            .  길            S  시작 위치       X  출구 (처음에는 닫힌 문, 바깥 테두리에 둔다)
//   M  미노타우르스 (방에 갇혀 있음)    B  창살 (지나갈 수 없음)
//   %  가짜 벽 (hiddenOpen일 때만 보이고 지나갈 수 있음)   ^  가시 구덩이 (pitsPassable일 때만 지나갈 수 있음)
//   L H A T  아이템 (등불, 헤르메스의 신발, 아르고스의 눈, 아리아드네의 실타래)
// 칸 (col, row)의 중심 = ((col + 0.5)·cell, 0, (row + 0.5)·cell). 바닥은 y = 0.
#include <string>
#include <vector>

class Maze {
public:
    float cell = 2.0f;           // 한 칸의 길이
    float wallHeight = 2.6f;     // 벽 높이
    bool  exitOpen = false;      // 출구 문이 열렸는가 (실타래)
    bool  hiddenOpen = false;    // 가짜 벽을 지나갈 수 있는가 (아르고스의 눈 투시 중)
    bool  pitsPassable = false;  // 가시 구덩이를 지나갈 수 있는가 (대쉬 중)

    bool Load(const char* path);
    int  Cols() const { return rows.empty() ? 0 : (int)rows[0].size(); }
    int  Rows() const { return (int)rows.size(); }
    char At(int col, int row) const;                 // 지도 밖은 '#'
    bool IsSolid(int col, int row) const;            // 지금 지나갈 수 없는 칸인가
    bool Find(char c, int& col, int& row) const;     // 처음 나오는 글자 위치
    void CellCenter(int col, int row, float& x, float& z) const;
    void CellOf(float x, float z, int& col, int& row) const;
    bool InPit(float x, float z) const;              // 이 점이 가시 구덩이 칸 안인가

    // 반지름 radius인 원(위에서 본 플레이어)이 지나갈 수 없는 칸의 상자와 겹치는가
    bool Collides(float x, float z, float radius) const;

    void Build();              // 텍스처와 벽·바닥·천장 디스플레이 리스트를 만든다 (창을 만든 뒤 한 번)
    void Draw(float time) const;               // 불투명한 것
    void DrawTransparent(float time) const;    // 투시 중인 가짜 벽 (불투명한 것을 모두 그린 뒤)
    unsigned StoneTexture() const { return texFloor; }   // 돌바닥 텍스처 (단상 등에 같이 쓴다)

private:
    std::vector<std::string> rows;
    unsigned list = 0;
    unsigned texBrick = 0, texFloor = 0, texBand = 0;
    void DrawWallBox(int col, int row) const;  // 벽 한 칸의 보이는 면 (텍스처 포함)
};
