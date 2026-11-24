#include "SOR.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
const float kAxisEps = 1e-4f;  // 이 값보다 반지름이 작으면 "회전축 위의 점"으로 본다
}

// ---------------------------------------------------------------------------
// SOR Mesh 생성
// ---------------------------------------------------------------------------
Mesh BuildSOR(const std::vector<Vec2>& input, int slices, float sweepDegrees)
{
    Mesh mesh;
    const bool closed = sweepDegrees >= 359.99f;       // 한 바퀴면 마지막 줄이 첫 줄과 이어진다
    if (input.size() < 2 || slices < (closed ? 3 : 1) || sweepDegrees <= 0.0f) return mesh;

    // 0번 점이 위쪽이 되게 정리 → 아래 삼각형 순서가 "바깥에서 볼 때 반시계"가 된다
    std::vector<Vec2> prof = input;
    if (prof.front().y < prof.back().y)
        std::reverse(prof.begin(), prof.end());

    const int   P    = (int)prof.size();
    const float sweep = closed ? 2.0f * (float)M_PI : sweepDegrees * (float)M_PI / 180.0f;
    const float step = sweep / (float)slices;          // 회전 각도 (라디안)
    const int   columns = closed ? slices : slices + 1; // 열린 모양은 끝 줄이 하나 더 필요하다

    // 1) 정점 생성: i번째 회전(θ = i·step)마다 프로파일 전체를 Y축 회전
    //    Y축 회전 행렬 적용 (프로파일은 z = 0 평면에 있음)
    //      x' =  x·cosθ + z·sinθ  =  r·cosθ
    //      y' =  y
    //      z' = -x·sinθ + z·cosθ  = -r·sinθ
    mesh.vertices.reserve((size_t)columns * P);
    for (int i = 0; i < columns; ++i) {
        const float c = std::cos(i * step);
        const float s = std::sin(i * step);
        for (int j = 0; j < P; ++j) {
            const float r = std::max(0.0f, prof[j].x);
            mesh.vertices.push_back(Vec3(r * c, prof[j].y, -r * s));
        }
    }

    // 2) 삼각형 index: 이웃한 두 회전 단계(i, i+1)와 이웃한 두 점(j, j+1)이 만드는 사각형을 삼각형 2개로
    //
    //      a(i, j) ─── c(i+1, j)        a, b, d  /  a, d, c
    //        │      ╲     │             (마지막 단계 i+1은 % slices 로 0번과 이어져 닫힘)
    //      b(i, j+1) ─ d(i+1, j+1)     (한 바퀴가 아니면 잇지 않고 열어 둔다)
    auto idx = [P](int i, int j) { return i * P + j; };
    for (int i = 0; i < slices; ++i) {
        const int next = closed ? (i + 1) % slices : i + 1;
        for (int j = 0; j < P - 1; ++j) {
            const int a = idx(i, j),    b = idx(i, j + 1);
            const int c = idx(next, j), d = idx(next, j + 1);
            const bool topOnAxis    = prof[j].x     <= kAxisEps;  // a == c (같은 점)
            const bool bottomOnAxis = prof[j + 1].x <= kAxisEps;  // b == d (같은 점)
            if (!bottomOnAxis) mesh.faces.push_back(Face{ { a, b, d } });
            if (!topOnAxis)    mesh.faces.push_back(Face{ { a, d, c } });
        }
    }

    // 3) 법선 계산
    mesh.ComputeNormals();

    // 회전축 위의 점은 회전 단계마다 복사본이 있으므로, 복사본들의 법선을 평균내서 꼭짓점을 매끄럽게
    for (int j = 0; j < P; ++j) {
        if (prof[j].x > kAxisEps) continue;
        Vec3 sum(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < columns; ++i) sum += mesh.normals[idx(i, j)];
        const Vec3 n = Normalize(sum);
        for (int i = 0; i < columns; ++i) mesh.normals[idx(i, j)] = n;
    }
    return mesh;
}

// ---------------------------------------------------------------------------
// Catmull-Rom 스플라인
//   p1 → p2 구간을 p0, p3까지 이용해 보간:
//   q(t) = 0.5 · [ 2p1 + (-p0 + p2)t + (2p0 - 5p1 + 4p2 - p3)t² + (-p0 + 3p1 - 3p2 + p3)t³ ]
// ---------------------------------------------------------------------------
std::vector<Vec2> CatmullRom(const std::vector<Vec2>& ctrl, int samples)
{
    const int n = (int)ctrl.size();
    if (n < 2 || samples < 1) return ctrl;

    // 곡선은 급하게 꺾이는 곳에서 제어점 바깥으로 튀어나갈 수 있다(overshoot).
    // 바닥(가장 낮은 제어점)보다 아래로 내려가면 게임에서 모델이 바닥에 묻히므로 막는다.
    float minY = ctrl[0].y;
    for (const Vec2& p : ctrl) minY = std::min(minY, p.y);

    std::vector<Vec2> out;
    out.reserve((size_t)(n - 1) * samples + 1);
    for (int i = 0; i < n - 1; ++i) {
        const Vec2& p0 = ctrl[i > 0 ? i - 1 : 0];      // 양 끝은 끝점을 한 번 더 쓴다
        const Vec2& p1 = ctrl[i];
        const Vec2& p2 = ctrl[i + 1];
        const Vec2& p3 = ctrl[i + 2 < n ? i + 2 : n - 1];
        for (int s = 0; s < samples; ++s) {
            const float t = (float)s / (float)samples, t2 = t * t, t3 = t2 * t;
            auto f = [&](float a0, float a1, float a2, float a3) {
                return 0.5f * (2.0f * a1 + (-a0 + a2) * t
                             + (2.0f * a0 - 5.0f * a1 + 4.0f * a2 - a3) * t2
                             + (-a0 + 3.0f * a1 - 3.0f * a2 + a3) * t3);
            };
            Vec2 q(f(p0.x, p1.x, p2.x, p3.x), f(p0.y, p1.y, p2.y, p3.y));
            if (q.x < 0.0f) q.x = 0.0f;  // 곡선이 회전축을 넘어가지 않게
            if (q.y < minY) q.y = minY;  // 가장 낮은 제어점보다 내려가지 않게
            out.push_back(q);
        }
    }
    out.push_back(ctrl.back());
    return out;
}

// ---------------------------------------------------------------------------
// .prof 저장 / 불러오기
// ---------------------------------------------------------------------------
bool SaveProfile(const char* path, const Profile& p)
{
    FILE* f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "# SOR profile (x = radius, y = height)\n");
    fprintf(f, "SLICES = %d\n", p.slices);
    fprintf(f, "CURVE = %d\n", p.curve ? 1 : 0);
    if (p.sweep < 359.99f) fprintf(f, "SWEEP = %.3f\n", p.sweep);
    fprintf(f, "POINTS = %d\n", (int)p.points.size());
    for (const Vec2& v : p.points)
        fprintf(f, "%.6f %.6f\n", v.x, v.y);
    fclose(f);
    return true;
}

bool LoadProfile(const char* path, Profile& out)
{
    FILE* f = fopen(path, "r");
    if (!f) return false;

    Profile p;
    int curve = 0, count = 0;
    char line[256];
    bool ok = fgets(line, sizeof(line), f) != nullptr                // 주석 줄
           && fscanf(f, " SLICES = %d", &p.slices) == 1
           && fscanf(f, " CURVE = %d", &curve) == 1;
    // SWEEP은 선택 항목 (없으면 360도), 그다음 POINTS
    char  key[32] = {};
    float value = 0.0f;
    ok = ok && fscanf(f, " %31[A-Z] = %f", key, &value) == 2;
    if (ok && strcmp(key, "SWEEP") == 0) {
        p.sweep = value;
        ok = value > 0.0f && value <= 360.0f
           && fscanf(f, " %31[A-Z] = %f", key, &value) == 2;
    }
    ok = ok && strcmp(key, "POINTS") == 0 && value >= 0.0f;
    if (ok) count = (int)value;
    for (int i = 0; ok && i < count; ++i) {
        Vec2 v;
        ok = fscanf(f, "%f %f", &v.x, &v.y) == 2;
        p.points.push_back(v);
    }
    fclose(f);
    if (!ok) return false;

    p.curve = curve != 0;
    out = p;
    return true;
}
