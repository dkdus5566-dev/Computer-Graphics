#pragma once
// 삼각형 Mesh 데이터 + .dat 파일 저장/불러오기 + OpenGL 그리기 (모델러·게임 공용)
#include <vector>
#include "Vec3.h"

// 삼각형 하나 = 정점 index 3개 (바깥에서 볼 때 반시계 방향)
struct Face {
    int v[3];
};

struct Mesh {
    std::vector<Vec3> vertices;  // 정점 위치
    std::vector<Vec3> normals;   // 정점마다 법선 (vertices와 개수 같음)
    std::vector<Face> faces;     // 삼각형 목록

    void Clear() { vertices.clear(); normals.clear(); faces.clear(); }
    bool Empty() const { return vertices.empty() || faces.empty(); }

    // 정점 법선 계산: 정점에 붙은 삼각형들의 면 법선(외적)을 넓이 가중 평균
    void ComputeNormals();

    // 경계 상자 (충돌 체크·배치용)
    void GetBounds(Vec3& minOut, Vec3& maxOut) const;
};

// .dat 형식 (텍스트)
//   VERTEX = n   다음 n줄: x y z
//   NORMAL = n   다음 n줄: nx ny nz   (없으면 불러올 때 계산)
//   FACE = m     다음 m줄: i0 i1 i2   (0부터 시작하는 index)
//   '#'으로 시작하는 줄은 주석
bool SaveMesh(const char* path, const Mesh& mesh);
bool LoadMesh(const char* path, Mesh& mesh);

// OpenGL 고정 파이프라인으로 그리기 (색·재질·변환은 호출하는 쪽에서 설정)
void DrawMesh(const Mesh& mesh);
void DrawMeshNormals(const Mesh& mesh, float length);
