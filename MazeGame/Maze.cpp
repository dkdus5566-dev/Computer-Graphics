#include "Maze.h"
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

bool Maze::Load(const char* path)
{
    std::ifstream in(path);
    if (!in) { printf("지도 파일을 열 수 없음: %s\n", path); return false; }
    rows.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == ';') continue;          // ';'로 시작하는 줄은 설명
        rows.push_back(line);
    }
    size_t w = 0;                                              // 줄 길이가 다르면 짧은 줄을 벽으로 채운다
    for (auto& r : rows) w = std::max(w, r.size());
    for (auto& r : rows) r.resize(w, '#');
    printf("지도 %s: %d x %d 칸\n", path, Cols(), Rows());
    return !rows.empty();
}

char Maze::At(int col, int row) const
{
    if (row < 0 || row >= Rows() || col < 0 || col >= Cols()) return '#';
    return rows[row][col];
}

bool Maze::IsSolid(int col, int row) const
{
    switch (At(col, row)) {
    case '#': case 'B': case 'M': return true;
    case 'X': return !exitOpen;
    case '%': return !hiddenOpen;
    case '^': return !pitsPassable;
    default:  return false;
    }
}

bool Maze::Find(char c, int& col, int& row) const
{
    for (row = 0; row < Rows(); ++row)
        for (col = 0; col < Cols(); ++col)
            if (rows[row][col] == c) return true;
    return false;
}

void Maze::CellCenter(int col, int row, float& x, float& z) const { x = (col + 0.5f) * cell; z = (row + 0.5f) * cell; }
void Maze::CellOf(float x, float z, int& col, int& row) const { col = (int)std::floor(x / cell); row = (int)std::floor(z / cell); }
bool Maze::InPit(float x, float z) const { int c, r; CellOf(x, z, c, r); return At(c, r) == '^'; }

// 원-상자 충돌: 상자 안에서 원의 중심에 가장 가까운 점까지 거리가 반지름보다 작으면 겹친다
bool Maze::Collides(float x, float z, float radius) const
{
    int c0, r0, c1, r1;
    CellOf(x - radius, z - radius, c0, r0);
    CellOf(x + radius, z + radius, c1, r1);
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            if (!IsSolid(c, r)) continue;
            float nx = std::clamp(x, c * cell, (c + 1) * cell);
            float nz = std::clamp(z, r * cell, (r + 1) * cell);
            float dx = x - nx, dz = z - nz;
            if (dx * dx + dz * dz < radius * radius) return true;
        }
    return false;
}

// ───────────────────────── 텍스처 (코드로 그림) ─────────────────────────
namespace {
unsigned rng = 12345;
float Rand() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 16777216.0f; }   // 0~1
float Hash(int a, int b) { unsigned h = (unsigned)(a * 73856093) ^ (unsigned)(b * 19349663); h = (h ^ (h >> 13)) * 1274126177u; return (h & 1023) / 1023.0f; }

struct Image {
    int w, h; std::vector<unsigned char> px;
    Image(int w_, int h_) : w(w_), h(h_), px(w_ * h_ * 3) {}
    void Set(int x, int y, float r, float g, float b)
    {
        x = (x % w + w) % w; y = (y % h + h) % h;
        unsigned char* p = &px[(y * w + x) * 3];
        p[0] = (unsigned char)std::clamp(r, 0.0f, 255.0f); p[1] = (unsigned char)std::clamp(g, 0.0f, 255.0f); p[2] = (unsigned char)std::clamp(b, 0.0f, 255.0f);
    }
    void Darken(int x, int y, float k)
    {
        x = (x % w + w) % w; y = (y % h + h) % h;
        unsigned char* p = &px[(y * w + x) * 3];
        for (int i = 0; i < 3; ++i) p[i] = (unsigned char)(p[i] * k);
    }
    // 금: 한 점에서 비틀비틀 이어지는 어두운 선 (황폐한 느낌)
    void Crack(int x, int y, int len)
    {
        float a = Rand() * 6.283f;
        for (int i = 0; i < len; ++i) { Darken(x, y, 0.55f); a += (Rand() - 0.5f) * 0.9f; x += (int)std::lround(std::cos(a)); y += (int)std::lround(std::sin(a)); }
    }
};

// 부드러운 얼룩용 저주파 잡음 (격자 값을 이중선형 보간)
float Smooth(float x, float y, int seed)
{
    int xi = (int)std::floor(x), yi = (int)std::floor(y); float fx = x - xi, fy = y - yi;
    auto v = [&](int a, int b) { return Hash(a + seed * 31, b + seed * 17); };
    float a = v(xi, yi) + (v(xi + 1, yi) - v(xi, yi)) * fx, b = v(xi, yi + 1) + (v(xi + 1, yi + 1) - v(xi, yi + 1)) * fx;
    return a + (b - a) * fy;
}

unsigned Upload(const Image& img)
{
    unsigned id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, img.w, img.h, GL_RGB, GL_UNSIGNED_BYTE, img.px.data());   // 멀리서 반짝임 방지
    return id;
}

// 황색 벽돌: 줄마다 반 장씩 어긋나게 쌓고, 벽돌마다 색을 조금씩 다르게, 얼룩·금·깨진 모서리
Image MakeBrick()
{
    const int W = 256, H = 256, bw = 64, bh = 32, mortar = 3;
    Image img(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            int row = y / bh, xs = x + (row % 2) * (bw / 2), col = xs / bw;
            int lx = xs % bw, ly = y % bh;
            float k = 0.85f + 0.3f * Hash(col * 7 + row, row * 13 + col);
            float stain = 0.75f + 0.35f * Smooth(x / 40.0f, y / 40.0f, 1);
            float n = (Rand() - 0.5f) * 18;
            if (lx < mortar || ly < mortar) img.Set(x, y, 120 * stain + n, 104 * stain + n, 80 * stain + n);       // 줄눈
            else img.Set(x, y, 205 * k * stain + n, 168 * k * stain + n, 104 * k * stain + n);                    // 벽돌
        }
    for (int i = 0; i < 14; ++i) img.Crack((int)(Rand() * W), (int)(Rand() * H), 20 + (int)(Rand() * 50));
    for (int i = 0; i < 10; ++i) {   // 깨져 나간 모서리
        int cx = (int)(Rand() * W), cy = (int)(Rand() * H), r = 4 + (int)(Rand() * 6);
        for (int dy = -r; dy <= r; ++dy) for (int dx = -r; dx <= r; ++dx) if (dx * dx + dy * dy <= r * r) img.Darken(cx + dx, cy + dy, 0.72f);
    }
    return img;
}

// 바닥: 큰 돌판 2×2, 먼지 얼룩과 금
Image MakeFloor()
{
    const int W = 256, H = 256, slab = 128;
    Image img(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            int sx = x / slab, sy = y / slab;
            float k = 0.85f + 0.25f * Hash(sx + 3, sy + 5);
            float dust = 0.7f + 0.4f * Smooth(x / 32.0f, y / 32.0f, 2);
            float n = (Rand() - 0.5f) * 16;
            bool grout = x % slab < 3 || y % slab < 3;
            float c = grout ? 0.55f : k * dust;
            img.Set(x, y, 168 * c + n, 146 * c + n, 112 * c + n);
        }
    for (int i = 0; i < 10; ++i) img.Crack((int)(Rand() * W), (int)(Rand() * H), 30 + (int)(Rand() * 60));
    return img;
}

// 그리스 문양(메안더) 띠: 8×8 칸 무늬를 가로로 반복
Image MakeBand()
{
    static const char* key[8] = { "1111111.", "1.....1.", "1.111.1.", "1.1.1.1.", "1.1...1.", "1.11111.", "1.......", "11111111" };
    const int W = 256, H = 64, s = 8;
    Image img(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float worn = 0.8f + 0.35f * Smooth(x / 24.0f, y / 24.0f, 3), n = (Rand() - 0.5f) * 14;
            bool line = key[7 - (y / s) % 8][(x / s) % 8] == '1';
            if (line) img.Set(x, y, 120 * worn + n, 45 * worn + n, 25 * worn + n);     // 붉은 갈색 선
            else      img.Set(x, y, 214 * worn + n, 160 * worn + n, 82 * worn + n);    // 황토 바탕
        }
    for (int i = 0; i < 6; ++i) img.Crack((int)(Rand() * W), (int)(Rand() * H), 15 + (int)(Rand() * 25));
    return img;
}

// 직사각형 면을 n×m개로 잘게 나눠 그린다. 고정 파이프라인 조명은 정점마다 계산하므로
// 면이 크면 플레이어가 든 점광원의 밝기 변화가 보이지 않는다.
// p0: 시작 모서리, u·v: 두 변 벡터, normal: 면 법선, (s0,t0)~(s1,t1): 텍스처 좌표 범위
void Quad(const float p0[3], const float u[3], const float v[3], const float normal[3], int n, int m,
          float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1)
{
    glNormal3fv(normal);
    glBegin(GL_QUADS);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j) {
            float a0 = (float)i / n, a1 = (float)(i + 1) / n, b0 = (float)j / m, b1 = (float)(j + 1) / m;
            float aa[4] = { a0, a1, a1, a0 }, bb[4] = { b0, b0, b1, b1 };
            for (int k = 0; k < 4; ++k) {
                glTexCoord2f(s0 + (s1 - s0) * aa[k], t0 + (t1 - t0) * bb[k]);
                glVertex3f(p0[0] + u[0] * aa[k] + v[0] * bb[k], p0[1] + u[1] * aa[k] + v[1] * bb[k], p0[2] + u[2] * aa[k] + v[2] * bb[k]);
            }
        }
    glEnd();
}

bool Open(char c) { return c != '#' && c != 'X'; }   // 벽 면을 그려야 하는 이웃 (가짜 벽 칸 안에서도 벽이 보이게)
}

// 벽 한 칸: 이웃이 열린 쪽 면만 그린다. 아래 띠(어둡게) + 벽돌 + 위 띠(그리스 문양)
void Maze::DrawWallBox(int c, int r) const
{
    const float s = cell, H = wallHeight, x0 = c * s, z0 = r * s;
    const float k = 0.9f + 0.2f * Hash(c, r);
    struct Side { int dc, dr; float p[3], u[3], n[3]; } sides[4] = {
        { 0, -1, { x0 + s, 0, z0 },     { -s, 0, 0 }, { 0, 0, -1 } },   // 북쪽 면
        { 0,  1, { x0, 0, z0 + s },     {  s, 0, 0 }, { 0, 0,  1 } },   // 남쪽 면
        { -1, 0, { x0, 0, z0 },         { 0, 0,  s }, { -1, 0, 0 } },   // 서쪽 면
        { 1,  0, { x0 + s, 0, z0 + s }, { 0, 0, -s }, { 1, 0, 0 } },    // 동쪽 면
    };
    for (auto& sd : sides) {
        char nb = At(c + sd.dc, r + sd.dr);
        if (!Open(nb) || (At(c, r) == '%' && nb == '%')) continue;
        glBindTexture(GL_TEXTURE_2D, texBrick);
        float base[3] = { sd.p[0], 0, sd.p[2] }, up1[3] = { 0, 0.30f, 0 };
        glColor3f(0.55f * k, 0.48f * k, 0.42f * k);
        Quad(base, sd.u, up1, sd.n, 4, 1, 0, 0, 1, 0.15f);   // 벽돌 하나 = 0.5 × 0.25
        float mid[3] = { sd.p[0], 0.30f, sd.p[2] }, up2[3] = { 0, H - 0.70f, 0 };
        glColor3f(1.0f * k, 0.95f * k, 0.88f * k);
        Quad(mid, sd.u, up2, sd.n, 4, 5, 0, 0.15f, 1, (H - 0.40f) / 2);
        glBindTexture(GL_TEXTURE_2D, texBand);
        float top[3] = { sd.p[0], H - 0.40f, sd.p[2] }, up3[3] = { 0, 0.40f, 0 };
        glColor3f(0.95f, 0.9f, 0.85f);
        Quad(top, sd.u, up3, sd.n, 4, 1, 0, 0, 1.25f, 1);   // 문양 한 칸 = 0.4 × 0.4
    }
}

void Maze::Build()
{
    rng = 12345;
    if (!texBrick) { texBrick = Upload(MakeBrick()); texFloor = Upload(MakeFloor()); texBand = Upload(MakeBand()); }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);   // 텍스처 색 × 조명 계산 색

    if (list) glDeleteLists(list, 1);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    GLfloat noSpec[] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
    glEnable(GL_TEXTURE_2D);
    const float s = cell, H = wallHeight, pitDepth = 1.2f;
    for (int r = 0; r < Rows(); ++r)
        for (int c = 0; c < Cols(); ++c) {
            const char ch = At(c, r);
            const float x0 = c * s, z0 = r * s, k = 0.85f + 0.3f * Hash(c, r);
            if (ch == '#') { DrawWallBox(c, r); continue; }
            if (ch == 'X') continue;
            // 천장
            glBindTexture(GL_TEXTURE_2D, texFloor);
            float cp[3] = { x0, H, z0 }, cu[3] = { s, 0, 0 }, cv[3] = { 0, 0, s }, down[3] = { 0, -1, 0 };
            glColor3f(0.62f, 0.56f, 0.50f);   // 천장: 바닥 텍스처를 조금 어둡게
            Quad(cp, cu, cv, down, 2, 2, 0, 0, 1, 1);
            if (ch == '^') {
                // 가시 구덩이: 바닥을 pitDepth만큼 낮추고 네 옆면과 가시를 그린다
                float bp[3] = { x0, -pitDepth, z0 + s }, bu[3] = { s, 0, 0 }, bv[3] = { 0, 0, -s }, up[3] = { 0, 1, 0 };
                glColor3f(0.35f, 0.3f, 0.26f);
                Quad(bp, bu, bv, up, 2, 2, 0, 0, 1, 1);
                struct Side { float p[3], u[3], n[3]; } sides[4] = {
                    { { x0, -pitDepth, z0 },         {  s, 0, 0 }, { 0, 0,  1 } },
                    { { x0 + s, -pitDepth, z0 + s }, { -s, 0, 0 }, { 0, 0, -1 } },
                    { { x0, -pitDepth, z0 + s },     { 0, 0, -s }, { 1, 0, 0 } },
                    { { x0 + s, -pitDepth, z0 },     { 0, 0,  s }, { -1, 0, 0 } },
                };
                glBindTexture(GL_TEXTURE_2D, texBrick);
                glColor3f(0.45f, 0.38f, 0.32f);
                for (auto& sd : sides) { float v[3] = { 0, pitDepth, 0 }; Quad(sd.p, sd.u, v, sd.n, 2, 2, 0, 0, 1, pitDepth / 2); }
                glDisable(GL_TEXTURE_2D);
                GLfloat sp[] = { 0.6f, 0.6f, 0.55f, 1 };
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sp); glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30);
                glColor3f(0.45f, 0.42f, 0.38f);
                for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 4; ++j) {
                        glPushMatrix();
                        glTranslatef(x0 + (i + 0.5f) * s / 4 + (Hash(i, j + r) - 0.5f) * 0.1f, -pitDepth, z0 + (j + 0.5f) * s / 4);
                        glRotatef(-90, 1, 0, 0);
                        glutSolidCone(0.09, 0.75 + 0.3 * Hash(c + i, j), 8, 1);
                        glPopMatrix();
                    }
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
                glEnable(GL_TEXTURE_2D);
                continue;
            }
            // 바닥 (가짜 벽 칸 포함)
            glBindTexture(GL_TEXTURE_2D, texFloor);
            float fp[3] = { x0, 0, z0 + s }, fu[3] = { s, 0, 0 }, fv[3] = { 0, 0, -s }, up[3] = { 0, 1, 0 };
            glColor3f(0.95f * k, 0.9f * k, 0.85f * k);
            Quad(fp, fu, fv, up, 4, 4, 0, 0, 1, 1);
            if (ch == 'B') {
                // 창살: 통로 방향에 수직인 청동 기둥 줄
                glDisable(GL_TEXTURE_2D);
                const bool alongX = Open(At(c, r - 1)) || Open(At(c, r + 1));   // 남북으로 열려 있으면 창살은 x 방향으로 늘어선다
                GLfloat sp[] = { 0.7f, 0.55f, 0.3f, 1 };
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sp); glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40);
                glColor3f(0.45f, 0.32f, 0.16f);
                GLUquadric* q = gluNewQuadric();
                for (int i = 0; i < 7; ++i) {
                    float t = (i + 0.5f) / 7 * s;
                    glPushMatrix();
                    glTranslatef(alongX ? x0 + t : x0 + s / 2, 0, alongX ? z0 + s / 2 : z0 + t);
                    glRotatef(-90, 1, 0, 0);
                    gluCylinder(q, 0.045, 0.045, H, 10, 1);
                    glPopMatrix();
                }
                for (float y : { 0.15f, H - 0.35f }) {   // 가로대
                    glPushMatrix();
                    glTranslatef(alongX ? x0 : x0 + s / 2, y, alongX ? z0 + s / 2 : z0);
                    if (alongX) glRotatef(90, 0, 1, 0);
                    gluCylinder(q, 0.06, 0.06, s, 10, 1);
                    glPopMatrix();
                }
                gluDeleteQuadric(q);
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
                glEnable(GL_TEXTURE_2D);
            }
        }
    glDisable(GL_TEXTURE_2D);
    glEndList();
}

void Maze::Draw(float time) const
{
    glCallList(list);

    // 가짜 벽: 평소에는 진짜 벽과 똑같이 보인다
    if (!hiddenOpen) {
        glEnable(GL_TEXTURE_2D);
        for (int r = 0; r < Rows(); ++r) for (int c = 0; c < Cols(); ++c) if (At(c, r) == '%') DrawWallBox(c, r);
        glDisable(GL_TEXTURE_2D);
    }

    // 출구: 닫혀 있으면 금빛 문, 열리면 바깥에서 들어오는 흰 빛
    int c, r;
    if (!Find('X', c, r)) return;
    const float s = cell, H = wallHeight, x0 = c * s, z0 = r * s;
    glPushMatrix();
    glTranslatef(x0 + s / 2, 0, z0 + s / 2);
    if (!exitOpen) {
        float glow = 0.25f + 0.15f * std::sin(time * 2.0f);
        GLfloat e[] = { glow, glow * 0.75f, glow * 0.3f, 1 }, sp[] = { 0.8f, 0.7f, 0.4f, 1 }, zero[] = { 0, 0, 0, 1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sp);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 40);
        glColor3f(0.75f, 0.58f, 0.22f);
        glPushMatrix(); glTranslatef(0, H / 2, 0); glScalef(s, H, s); glutSolidCube(1.0); glPopMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
    } else {
        glDisable(GL_LIGHTING); glDisable(GL_FOG);
        glColor3f(1.0f, 0.97f, 0.85f);
        glPushMatrix(); glTranslatef(0, H / 2, 0); glScalef(s * 0.98f, H, s * 0.98f); glutSolidCube(1.0); glPopMatrix();
        glEnable(GL_FOG); glEnable(GL_LIGHTING);
    }
    glPopMatrix();
}

void Maze::DrawTransparent(float time) const
{
    if (!hiddenOpen) return;
    // 투시 중: 가짜 벽이 일렁이는 반투명 빛으로 보인다
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glDepthMask(GL_FALSE); glDisable(GL_LIGHTING);
    const float a = 0.25f + 0.1f * std::sin(time * 5.0f);
    for (int r = 0; r < Rows(); ++r)
        for (int c = 0; c < Cols(); ++c) {
            if (At(c, r) != '%') continue;
            glColor4f(0.5f, 0.75f, 1.0f, a);
            // 칸과 똑같은 크기면 옆 벽면·바닥·천장과 같은 평면에 겹쳐 깊이 값이 번갈아 이기며 지직거린다(z-fighting).
            // 상자를 안쪽으로 조금 줄여 겹치는 면을 없앤다.
            const float inset = 0.06f;
            glPushMatrix();
            glTranslatef((c + 0.5f) * cell, wallHeight / 2, (r + 0.5f) * cell);
            glScalef(cell - 2 * inset, wallHeight - 2 * inset, cell - 2 * inset);
            glutSolidCube(1.0);
            glPopMatrix();
        }
    glEnable(GL_LIGHTING); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}
