#pragma once
// SOR(Surface of Revolution, 회전체) 생성 — 2D 프로파일을 Y축으로 회전시켜 삼각형 Mesh를 만든다.
#include <vector>
#include "Mesh.h"

// 프로파일 편집 상태 (모델러가 .prof 파일로 저장 → 다시 열어서 수정 가능)
struct Profile {
    std::vector<Vec2> points;  // 제어점: x = 반지름(>= 0), y = 높이
    int  slices = 24;          // 한 바퀴를 몇 번에 나눠 회전할지 (회전 각도 = 360 / slices)
    bool curve  = true;        // 제어점 사이를 Catmull-Rom 곡선으로 보간할지
};

// profile 점들을 Y축으로 (360 / slices)도씩 회전시켜 Mesh를 만든다.
//   - 정점 index = i * P + j   (i: 회전 단계, j: 프로파일 점 번호, P: 프로파일 점 개수)
//   - 회전축 위의 점(x = 0)은 찌그러진 삼각형을 만들지 않도록 처리
//   - 점이 2개 미만이거나 slices < 3 이면 빈 Mesh
Mesh BuildSOR(const std::vector<Vec2>& profile, int slices);

// Catmull-Rom 스플라인: 제어점을 반드시 지나는 부드러운 곡선 점들을 만든다.
//   samplesPerSegment: 제어점 두 개 사이를 몇 개로 나눌지
std::vector<Vec2> CatmullRom(const std::vector<Vec2>& controlPoints, int samplesPerSegment);

// .prof 파일 (텍스트)
//   SLICES = n / CURVE = 0|1 / POINTS = n  다음 n줄: x y
bool SaveProfile(const char* path, const Profile& profile);
bool LoadProfile(const char* path, Profile& profile);
