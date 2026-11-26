#include "ItemModel.h"
#include "Mesh.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

// 4×4 행렬, OpenGL과 같은 열 우선(column-major) 순서: m[col*4 + row]
struct Mat4 { float m[16]; };

Mat4 Identity() { Mat4 a{}; a.m[0] = a.m[5] = a.m[10] = a.m[15] = 1; return a; }

Mat4 Mul(const Mat4& a, const Mat4& b)   // a·b (b를 먼저 적용)
{
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr)
            for (int k = 0; k < 4; ++k) r.m[c * 4 + rr] += a.m[k * 4 + rr] * b.m[c * 4 + k];
    return r;
}

Mat4 Translate(float x, float y, float z) { Mat4 a = Identity(); a.m[12] = x; a.m[13] = y; a.m[14] = z; return a; }
Mat4 Scale(float x, float y, float z)     { Mat4 a = Identity(); a.m[0] = x; a.m[5] = y; a.m[10] = z; return a; }

// 축 (x,y,z) 둘레로 deg도 회전 — glRotatef 설명서의 로드리게스 회전 행렬
Mat4 Rotate(float deg, float x, float y, float z)
{
    float len = std::sqrt(x * x + y * y + z * z);
    if (len < 1e-8f) return Identity();
    x /= len; y /= len; z /= len;
    const float r = deg * (float)M_PI / 180.0f, c = std::cos(r), s = std::sin(r), t = 1 - c;
    Mat4 a = Identity();
    a.m[0] = t * x * x + c;     a.m[4] = t * x * y - s * z; a.m[8]  = t * x * z + s * y;
    a.m[1] = t * x * y + s * z; a.m[5] = t * y * y + c;     a.m[9]  = t * y * z - s * x;
    a.m[2] = t * x * z - s * y; a.m[6] = t * y * z + s * x; a.m[10] = t * z * z + c;
    return a;
}

Vec3 Apply(const Mat4& a, const Vec3& p)
{
    return Vec3(a.m[0] * p.x + a.m[4] * p.y + a.m[8] * p.z + a.m[12],
                a.m[1] * p.x + a.m[5] * p.y + a.m[9] * p.z + a.m[13],
                a.m[2] * p.x + a.m[6] * p.y + a.m[10] * p.z + a.m[14]);
}

// 위쪽 3×3의 행렬식. 음수면 거울 반전(음수 크기)이 들어간 것 → 삼각형 감김 방향이 뒤집힌다
float Det3(const Mat4& a)
{
    return a.m[0] * (a.m[5] * a.m[10] - a.m[9] * a.m[6])
         - a.m[4] * (a.m[1] * a.m[10] - a.m[9] * a.m[2])
         + a.m[8] * (a.m[1] * a.m[6] - a.m[5] * a.m[2]);
}

struct Band { float y0, y1, rgb[3]; };

struct Part {
    std::string file;
    float rgba[4] = { 1, 1, 1, 1 };
    float spec = 0.1f, glow = 0.0f;
    Mat4 local = Identity();
    std::vector<float> repeatY;
    std::vector<Band> bands;
};

// 같은 .dat을 여러 아이템·부품이 써도 한 번만 읽는다
const Mesh* GetMesh(const std::string& file)
{
    static std::map<std::string, Mesh> cache;
    auto it = cache.find(file);
    if (it != cache.end()) return it->second.Empty() ? nullptr : &it->second;
    Mesh& m = cache[file];
    if (!LoadMesh(("Models/" + file).c_str(), m)) printf("  부품을 읽지 못함: Models/%s\n", file.c_str());
    return m.Empty() ? nullptr : &m;
}

bool ParseColor(const std::string& s, float rgb[3])
{
    unsigned v;
    if (s.size() != 7 || s[0] != '#' || sscanf(s.c_str() + 1, "%x", &v) != 1) return false;
    rgb[0] = ((v >> 16) & 255) / 255.0f; rgb[1] = ((v >> 8) & 255) / 255.0f; rgb[2] = (v & 255) / 255.0f;
    return true;
}

void DrawPart(const Part& p, const Mesh& mesh, const Mat4& M)
{
    GLfloat spec[] = { p.spec, p.spec * 0.92f, p.spec * 0.75f, 1 };
    GLfloat emis[] = { p.rgba[0] * p.glow, p.rgba[1] * p.glow, p.rgba[2] * p.glow, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emis);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, p.spec > 0.3f ? 45.0f : 10.0f);
    glFrontFace(Det3(M) < 0 ? GL_CW : GL_CCW);
    glPushMatrix();
    glMultMatrixf(M.m);
    if (p.bands.empty()) {
        glColor4fv(p.rgba);
        DrawMesh(mesh);
    } else {
        glBegin(GL_TRIANGLES);
        for (const Face& f : mesh.faces) {
            const float cy = (mesh.vertices[f.v[0]].y + mesh.vertices[f.v[1]].y + mesh.vertices[f.v[2]].y) / 3;
            const float* c = p.rgba;
            for (const Band& b : p.bands) if (cy >= b.y0 && cy <= b.y1) c = b.rgb;
            glColor4f(c[0], c[1], c[2], p.rgba[3]);
            for (int k = 0; k < 3; ++k) {
                const Vec3& n = mesh.normals[f.v[k]]; const Vec3& v = mesh.vertices[f.v[k]];
                glNormal3f(n.x, n.y, n.z); glVertex3f(v.x, v.y, v.z);
            }
        }
        glEnd();
    }
    glPopMatrix();
    glFrontFace(GL_CCW);
}

} // namespace

bool ItemModel::Load(const char* path)
{
    std::ifstream in(path);
    if (!in) { printf("조립 파일 없음: %s\n", path); return false; }

    std::vector<Part> parts;
    std::string line;
    int lineNo = 0;
    bool ok = true;
    while (std::getline(in, line)) {
        ++lineNo;
        std::istringstream ss(line);
        std::string op;
        if (!(ss >> op) || op[0] == '#') continue;
        auto fail = [&](const char* why) { printf("  %s:%d %s: %s\n", path, lineNo, why, line.c_str()); ok = false; };
        if (op == "PART") {
            Part p; std::string color;
            if (!(ss >> p.file >> color) || !ParseColor(color, p.rgba)) { fail("PART 형식 오류"); continue; }
            // 뒤의 세 값은 생략 가능. (읽기에 실패하면 C++가 0을 넣으므로 성공했을 때만 바꾼다)
            float v;
            if (ss >> v) { p.rgba[3] = v; if (ss >> v) { p.spec = v; if (ss >> v) p.glow = v; } }
            parts.push_back(p);
            continue;
        }
        if (parts.empty()) { fail("PART 앞에 변환이 있음"); continue; }
        Part& p = parts.back();
        float a, x, y, z;
        if (op == "S" && ss >> x >> y >> z)              p.local = Mul(Scale(x, y, z), p.local);
        else if (op == "T" && ss >> x >> y >> z)         p.local = Mul(Translate(x, y, z), p.local);
        else if (op == "RX" && ss >> a)                  p.local = Mul(Rotate(a, 1, 0, 0), p.local);
        else if (op == "RY" && ss >> a)                  p.local = Mul(Rotate(a, 0, 1, 0), p.local);
        else if (op == "RZ" && ss >> a)                  p.local = Mul(Rotate(a, 0, 0, 1), p.local);
        else if (op == "R" && ss >> a >> x >> y >> z)    p.local = Mul(Rotate(a, x, y, z), p.local);
        else if (op == "REPEAT_Y")                       { while (ss >> a) p.repeatY.push_back(a); }
        else if (op == "BAND") {
            Band b; std::string color;
            if (ss >> b.y0 >> b.y1 >> color && ParseColor(color, b.rgb)) p.bands.push_back(b);
            else fail("BAND 형식 오류");
        }
        else fail("알 수 없는 줄");
    }

    // 부품마다 최종 행렬(반복 포함)을 만들고 경계 상자를 계산한다
    struct Instance { const Part* part; const Mesh* mesh; Mat4 M; };
    std::vector<Instance> list;
    boundsMin = Vec3(1e9f, 1e9f, 1e9f); boundsMax = Vec3(-1e9f, -1e9f, -1e9f);
    triangles = 0;
    for (const Part& p : parts) {
        const Mesh* mesh = GetMesh(p.file);
        if (!mesh) { ok = false; continue; }
        std::vector<float> angles = p.repeatY.empty() ? std::vector<float>{ 0.0f } : p.repeatY;
        for (float a : angles) {
            Instance inst{ &p, mesh, Mul(Rotate(a, 0, 1, 0), p.local) };
            for (const Vec3& v : mesh->vertices) {
                Vec3 q = Apply(inst.M, v);
                boundsMin = Vec3(std::fmin(boundsMin.x, q.x), std::fmin(boundsMin.y, q.y), std::fmin(boundsMin.z, q.z));
                boundsMax = Vec3(std::fmax(boundsMax.x, q.x), std::fmax(boundsMax.y, q.y), std::fmax(boundsMax.z, q.z));
            }
            triangles += (int)mesh->faces.size();
            list.push_back(inst);
        }
    }
    if (list.empty()) { printf("조립 파일에 그릴 부품이 없음: %s\n", path); return false; }

    GLfloat zero[] = { 0, 0, 0, 1 };
    // 불투명 부품
    listOpaque = glGenLists(1);
    glNewList(listOpaque, GL_COMPILE);
    for (const Instance& i : list) if (i.part->rgba[3] >= 1.0f) DrawPart(*i.part, *i.mesh, i.M);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glEndList();
    // 반투명 부품: 깊이 버퍼에 쓰지 않고 섞는다. 뒷면 → 앞면 순서로 그려 안쪽이 비쳐 보이게 한다
    listTransparent = glGenLists(1);
    glNewList(listTransparent, GL_COMPILE);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); glEnable(GL_CULL_FACE);
    for (const Instance& i : list) {
        if (i.part->rgba[3] >= 1.0f) continue;
        glCullFace(GL_FRONT); DrawPart(*i.part, *i.mesh, i.M);
        glCullFace(GL_BACK);  DrawPart(*i.part, *i.mesh, i.M);
    }
    glDisable(GL_CULL_FACE); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glEndList();

    loaded = true;
    printf("아이템 %s: 부품 %d개, 삼각형 %d개, 높이 %.3f%s\n", path, (int)list.size(), triangles,
           boundsMax.y - boundsMin.y, ok ? "" : " (오류 있음, 위 메시지 확인)");
    return true;
}

void ItemModel::DrawOpaque() const      { if (loaded) glCallList(listOpaque); }
void ItemModel::DrawTransparent() const { if (loaded) glCallList(listTransparent); }
