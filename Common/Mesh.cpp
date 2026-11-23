#include "Mesh.h"

#include <GL/freeglut.h>
#include <cstdio>
#include <cstring>
#include <cfloat>

// ---------------------------------------------------------------------------
// 법선 계산
// ---------------------------------------------------------------------------
void Mesh::ComputeNormals()
{
    normals.assign(vertices.size(), Vec3(0.0f, 0.0f, 0.0f));

    for (const Face& f : faces) {
        const Vec3& a = vertices[f.v[0]];
        const Vec3& b = vertices[f.v[1]];
        const Vec3& c = vertices[f.v[2]];
        // (b - a) × (c - a): 반시계 순서면 바깥쪽을 향하고, 길이 = 삼각형 넓이 × 2
        Vec3 faceN = Cross(b - a, c - a);
        for (int k = 0; k < 3; ++k)
            normals[f.v[k]] += faceN;  // 넓이가 큰 면일수록 영향이 큼
    }
    for (Vec3& n : normals)
        n = Normalize(n);
}

void Mesh::GetBounds(Vec3& minOut, Vec3& maxOut) const
{
    if (vertices.empty()) {
        minOut = maxOut = Vec3(0.0f, 0.0f, 0.0f);
        return;
    }
    minOut = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    maxOut = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (const Vec3& v : vertices) {
        if (v.x < minOut.x) minOut.x = v.x;
        if (v.y < minOut.y) minOut.y = v.y;
        if (v.z < minOut.z) minOut.z = v.z;
        if (v.x > maxOut.x) maxOut.x = v.x;
        if (v.y > maxOut.y) maxOut.y = v.y;
        if (v.z > maxOut.z) maxOut.z = v.z;
    }
}

// ---------------------------------------------------------------------------
// 파일 저장
// ---------------------------------------------------------------------------
bool SaveMesh(const char* path, const Mesh& mesh)
{
    FILE* f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "# SOR mesh (Computer-Graphics team format)\n");
    fprintf(f, "VERTEX = %d\n", (int)mesh.vertices.size());
    for (const Vec3& v : mesh.vertices)
        fprintf(f, "%.6f %.6f %.6f\n", v.x, v.y, v.z);

    fprintf(f, "NORMAL = %d\n", (int)mesh.normals.size());
    for (const Vec3& n : mesh.normals)
        fprintf(f, "%.6f %.6f %.6f\n", n.x, n.y, n.z);

    fprintf(f, "FACE = %d\n", (int)mesh.faces.size());
    for (const Face& t : mesh.faces)
        fprintf(f, "%d %d %d\n", t.v[0], t.v[1], t.v[2]);

    fclose(f);
    return true;
}

// ---------------------------------------------------------------------------
// 파일 불러오기
// ---------------------------------------------------------------------------
namespace {

// 주석(#)·빈 줄을 건너뛰고 내용이 있는 다음 줄을 읽는다.
bool ReadContentLine(FILE* f, char* buf, int size)
{
    while (fgets(buf, size, f)) {
        const char* p = buf;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') continue;
        return true;
    }
    return false;
}

// "KEY = n" 형식의 머리줄 읽기
bool ReadHeader(FILE* f, char* key, int& count)
{
    char line[256];
    if (!ReadContentLine(f, line, sizeof(line))) return false;
    return sscanf(line, " %31[A-Za-z_] = %d", key, &count) == 2;
}

} // namespace

bool LoadMesh(const char* path, Mesh& mesh)
{
    FILE* f = fopen(path, "r");
    if (!f) return false;

    Mesh m;
    bool ok = true;
    char key[32];
    int count = 0;
    char line[256];

    while (ok && ReadHeader(f, key, count)) {
        if (count < 0) { ok = false; break; }

        if (strcmp(key, "VERTEX") == 0 || strcmp(key, "NORMAL") == 0) {
            std::vector<Vec3>& dst = (key[0] == 'V') ? m.vertices : m.normals;
            dst.resize(count);
            for (int i = 0; i < count && ok; ++i) {
                ok = ReadContentLine(f, line, sizeof(line)) &&
                     sscanf(line, "%f %f %f", &dst[i].x, &dst[i].y, &dst[i].z) == 3;
            }
        }
        else if (strcmp(key, "FACE") == 0) {
            m.faces.resize(count);
            for (int i = 0; i < count && ok; ++i) {
                Face& t = m.faces[i];
                ok = ReadContentLine(f, line, sizeof(line)) &&
                     sscanf(line, "%d %d %d", &t.v[0], &t.v[1], &t.v[2]) == 3;
            }
        }
        else {
            ok = false;  // 모르는 항목
        }
    }
    fclose(f);

    if (!ok || m.vertices.empty()) return false;

    // index 범위 검사 (잘못된 파일로 프로그램이 죽지 않게)
    const int n = (int)m.vertices.size();
    for (const Face& t : m.faces)
        for (int k = 0; k < 3; ++k)
            if (t.v[k] < 0 || t.v[k] >= n) return false;

    // NORMAL이 없거나 개수가 맞지 않으면 직접 계산 (교수님 예시 형식 호환)
    if (m.normals.size() != m.vertices.size())
        m.ComputeNormals();

    mesh = m;
    return true;
}

// ---------------------------------------------------------------------------
// 그리기
// ---------------------------------------------------------------------------
void DrawMesh(const Mesh& mesh)
{
    const bool hasNormals = mesh.normals.size() == mesh.vertices.size();
    glBegin(GL_TRIANGLES);
    for (const Face& t : mesh.faces) {
        for (int k = 0; k < 3; ++k) {
            const int i = t.v[k];
            if (hasNormals) glNormal3f(mesh.normals[i].x, mesh.normals[i].y, mesh.normals[i].z);
            glVertex3f(mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
        }
    }
    glEnd();
}

void DrawMeshNormals(const Mesh& mesh, float length)
{
    if (mesh.normals.size() != mesh.vertices.size()) return;
    glBegin(GL_LINES);
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const Vec3& p = mesh.vertices[i];
        const Vec3 q = p + mesh.normals[i] * length;
        glVertex3f(p.x, p.y, p.z);
        glVertex3f(q.x, q.y, q.z);
    }
    glEnd();
}
