#pragma once
// 아이템 조립 파일(Models/<아이템>.item)을 읽어, 부품 .dat들을 변환·색과 함께 그린다.
//
// .item 형식 (텍스트, '#'으로 시작하는 줄은 주석)
//   PART <부품.dat> <#RRGGBB> [불투명도=1] [광택=0.1] [발광=0]   새 부품 시작
//   S x y z          크기
//   RX a / RY a / RZ a    X·Y·Z축 회전 (도)
//   R a x y z        임의의 축 (x,y,z)으로 a도 회전 (glRotatef와 같음)
//   T x y z          이동
//   REPEAT_Y a1 a2 … 위 변환을 마친 부품을 원점 기준 Y축으로 a1, a2…도 돌려 여러 개 놓는다
//   BAND y0 y1 #RRGGBB    부품 원래 좌표에서 삼각형 중심 높이가 y0~y1이면 이 색 (띠 장식)
// 변환은 **적은 순서대로** 정점에 적용된다 (예: S → RX → T).
// OpenGL로 쓰면 반대 순서로 호출한 것과 같다: glTranslatef → glRotatef → glScalef.
#include "Vec3.h"

class ItemModel {
public:
    bool Load(const char* path);    // 창(OpenGL)을 만든 뒤 호출. 실패하면 false
    void DrawOpaque() const;        // 불투명 부품
    void DrawTransparent() const;   // 반투명 부품 — 장면의 불투명한 것을 모두 그린 뒤 호출
    bool Loaded() const { return loaded; }

    Vec3 boundsMin, boundsMax;      // 조립된 전체 모양의 경계 상자 (게임에서 크기 맞추기·배치용)
    int  triangles = 0;

private:
    bool loaded = false;
    unsigned listOpaque = 0, listTransparent = 0;   // 디스플레이 리스트 (한 번 만들어 두고 매 프레임 호출)
};
