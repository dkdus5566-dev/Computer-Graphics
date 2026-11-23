// SOR 모델러
//   왼쪽 화면: 2D 프로파일 편집 (glOrtho 2D)  — 점을 찍으면
//   오른쪽 화면: Y축 회전체(SOR) 3D Mesh가 실시간으로 생성된다 (gluPerspective)
//   S 키: Models/<이름>.dat (Mesh) + Models/<이름>.prof (프로파일) 저장
#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <direct.h>   // _mkdir
#include <iostream>
#include <string>

#include "Mesh.h"
#include "SOR.h"

namespace {

// ---------------------------------------------------------------------------
// 상태
// ---------------------------------------------------------------------------
int     gWinW = 1200, gWinH = 640;
Profile gProfile;                 // 제어점 · 분할 수 · 곡선 사용 여부
Mesh    gMesh;                    // 현재 프로파일로 만든 3D Mesh
bool    gDirty = true;            // 프로파일이 바뀌면 true → 그리기 전에 Mesh 재생성
int     gDragIndex = -1;          // 드래그 중인 제어점 번호

bool gShowSolid = true, gShowWire = true, gShowNormals = false;

// 3D 미리보기 카메라 (대상점을 중심으로 도는 구면 좌표)
float gYaw = 35.0f, gPitch = 20.0f, gDist = 2.6f;
bool  gRotating = false;
int   gLastX = 0, gLastY = 0;

std::string gStatus = "Ready";

const int   kCurveSamples = 8;      // 제어점 사이 곡선 분할 수
const float kViewH        = 1.5f;   // 편집 화면에 보이는 높이 (월드 단위)
const float kViewYMin     = -0.15f;
const float kPickRadius   = 0.035f; // 이 거리 안의 점을 클릭하면 선택
const float kAxisSnap     = 0.015f; // 회전축에 이만큼 가까우면 x = 0 으로 붙임
const int   kMinSlices = 3, kMaxSlices = 120;

int HalfW() { return gWinW / 2; }

// ---------------------------------------------------------------------------
// 좌표 변환
// ---------------------------------------------------------------------------
// 편집 화면의 월드 범위: 높이는 고정, 가로는 뷰포트 비율에 맞춤 (찌그러짐 방지)
void EditorBounds(float& l, float& r, float& b, float& t)
{
    const float aspect = (float)HalfW() / (float)std::max(gWinH, 1);
    const float w = kViewH * aspect;
    l = -w * 0.5f;  r = w * 0.5f;
    b = kViewYMin;  t = kViewYMin + kViewH;
}

// 마우스(창 좌표: 왼쪽 위가 (0,0), y는 아래로 증가) → 편집 화면 월드 좌표 (y는 위로 증가)
Vec2 MouseToWorld(int mx, int my)
{
    float l, r, b, t;
    EditorBounds(l, r, b, t);
    const float u = (float)mx / (float)HalfW();
    const float v = 1.0f - (float)my / (float)gWinH;  // y축 뒤집기
    return Vec2(l + u * (r - l), b + v * (t - b));
}

// 프로파일 점은 회전축 오른쪽(x >= 0)에만, 축 근처면 축에 붙인다
Vec2 ClampToProfile(Vec2 p)
{
    if (p.x < kAxisSnap) p.x = 0.0f;
    return p;
}

int FindNearestPoint(const Vec2& p, float radius)
{
    int best = -1;
    float bestD2 = radius * radius;
    for (int i = 0; i < (int)gProfile.points.size(); ++i) {
        const float dx = gProfile.points[i].x - p.x, dy = gProfile.points[i].y - p.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 <= bestD2) { bestD2 = d2; best = i; }
    }
    return best;
}

// ---------------------------------------------------------------------------
// Mesh 재생성
// ---------------------------------------------------------------------------
std::vector<Vec2> SampledProfile()
{
    if (gProfile.curve && gProfile.points.size() >= 3)
        return CatmullRom(gProfile.points, kCurveSamples);
    return gProfile.points;
}

void RebuildIfDirty()
{
    if (!gDirty) return;
    gMesh = BuildSOR(SampledProfile(), gProfile.slices);
    gDirty = false;
}

void MarkDirty()
{
    gDirty = true;
    glutPostRedisplay();
}

void SetSlices(int n)
{
    gProfile.slices = std::clamp(n, kMinSlices, kMaxSlices);
    MarkDirty();
}

// ---------------------------------------------------------------------------
// 텍스트
// ---------------------------------------------------------------------------
void DrawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_12)
{
    glRasterPos2f(x, y);
    glutBitmapString(font, (const unsigned char*)s.c_str());
}

// 현재 뷰포트를 픽셀 좌표계로 (왼쪽 아래가 (0,0))
void BeginPixel2D(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

std::string Fmt(const char* fmt, double a, double b = 0, double c = 0)
{
    char buf[256];
    snprintf(buf, sizeof(buf), fmt, a, b, c);
    return buf;
}

// ---------------------------------------------------------------------------
// 왼쪽: 2D 프로파일 편집 화면
// ---------------------------------------------------------------------------
void DrawPolyline(const std::vector<Vec2>& pts, float mirror)
{
    glBegin(GL_LINE_STRIP);
    for (const Vec2& p : pts) glVertex2f(p.x * mirror, p.y);
    glEnd();
}

void DrawEditor()
{
    const int w = HalfW(), h = gWinH;
    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);
    glClearColor(0.12f, 0.13f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    float l, r, b, t;
    EditorBounds(l, r, b, t);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(l, r, b, t);  // 마우스 좌표와 같은 월드 좌표계
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 격자 (0.1 간격)
    glColor3f(0.19f, 0.20f, 0.23f);
    glBegin(GL_LINES);
    for (int k = (int)std::floor(l * 10); k <= (int)std::ceil(r * 10); ++k) {
        glVertex2f(k * 0.1f, b);  glVertex2f(k * 0.1f, t);
    }
    for (int k = (int)std::floor(b * 10); k <= (int)std::ceil(t * 10); ++k) {
        glVertex2f(l, k * 0.1f);  glVertex2f(r, k * 0.1f);
    }
    glEnd();

    // 바닥 y = 0
    glColor3f(0.50f, 0.50f, 0.55f);
    glBegin(GL_LINES); glVertex2f(l, 0.0f); glVertex2f(r, 0.0f); glEnd();
    // 높이 기준 y = 1 (팀 좌표 규칙: 모델 높이 ≈ 1)
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0F0F);
    glColor3f(0.55f, 0.50f, 0.30f);
    glBegin(GL_LINES); glVertex2f(l, 1.0f); glVertex2f(r, 1.0f); glEnd();
    glDisable(GL_LINE_STIPPLE);
    // 회전축 (Y축)
    glLineWidth(2.0f);
    glColor3f(0.90f, 0.35f, 0.30f);
    glBegin(GL_LINES); glVertex2f(0.0f, b); glVertex2f(0.0f, t); glEnd();
    glLineWidth(1.0f);

    glColor3f(0.60f, 0.60f, 0.65f);
    DrawText(r - 0.14f, 0.012f, "y = 0");
    DrawText(r - 0.14f, 1.012f, "y = 1");
    glColor3f(0.90f, 0.45f, 0.40f);
    DrawText(0.012f, t - 0.05f, "Y axis");

    const std::vector<Vec2> curve = SampledProfile();
    // 반대쪽 실루엣 (회전했을 때 모양 미리보기)
    glColor3f(0.35f, 0.36f, 0.42f);
    DrawPolyline(curve, -1.0f);
    // 프로파일 선
    glLineWidth(2.0f);
    glColor3f(0.95f, 0.80f, 0.35f);
    DrawPolyline(curve, 1.0f);
    glLineWidth(1.0f);
    // 제어점을 직선으로 이은 선 (곡선 모드일 때 비교용)
    if (gProfile.curve) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x3333);
        glColor3f(0.45f, 0.45f, 0.50f);
        DrawPolyline(gProfile.points, 1.0f);
        glDisable(GL_LINE_STIPPLE);
    }

    // 제어점과 번호
    glPointSize(9.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < (int)gProfile.points.size(); ++i) {
        if (i == gDragIndex) glColor3f(1.0f, 0.4f, 0.3f);
        else                 glColor3f(0.35f, 0.75f, 1.0f);
        glVertex2f(gProfile.points[i].x, gProfile.points[i].y);
    }
    glEnd();
    glPointSize(1.0f);
    glColor3f(0.85f, 0.88f, 0.95f);
    for (int i = 0; i < (int)gProfile.points.size(); ++i)
        DrawText(gProfile.points[i].x + 0.015f, gProfile.points[i].y + 0.015f, std::to_string(i));

    // 안내
    BeginPixel2D(w, h);
    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(12, (float)h - 24, "2D PROFILE", GLUT_BITMAP_HELVETICA_18);
    glColor3f(0.70f, 0.72f, 0.78f);
    DrawText(12, (float)h - 44, "x = radius, y = height  /  rotation axis = Y (red)");
    DrawText(12, 52, "L-click: add point   L-drag: move   R-click: delete");
    DrawText(12, 34, "U: undo last   X: clear all   C: curve on/off");
    glColor3f(0.95f, 0.80f, 0.35f);
    DrawText(12, 14, "points " + std::to_string(gProfile.points.size()) + "   curve "
                     + (gProfile.curve ? "ON (Catmull-Rom)" : "OFF (straight)"));
}

// ---------------------------------------------------------------------------
// 오른쪽: 3D 미리보기
// ---------------------------------------------------------------------------
void SetupLighting()
{
    // 방향광 (w = 0) — 뷰 변환 후에 설정해서 월드에 고정
    const GLfloat pos[]  = { 2.0f, 3.0f, 2.5f, 0.0f };
    const GLfloat amb[]  = { 0.25f, 0.25f, 0.25f, 1.0f };
    const GLfloat dif[]  = { 0.85f, 0.85f, 0.85f, 1.0f };
    const GLfloat spec[] = { 0.50f, 0.50f, 0.50f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);  // 열린 모양의 안쪽 면도 밝게

    const GLfloat matSpec[] = { 0.30f, 0.30f, 0.30f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);
}

void DrawPreview()
{
    const int x0 = HalfW(), w = gWinW - x0, h = gWinH;
    glViewport(x0, 0, w, h);
    glScissor(x0, 0, w, h);
    glClearColor(0.20f, 0.21f, 0.24f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Projection: 원근 투영
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, (double)w / (double)std::max(h, 1), 0.05, 100.0);

    // View: 구면 좌표(yaw, pitch, dist) → 카메라 위치
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const float yaw = gYaw * (float)M_PI / 180.0f, pitch = gPitch * (float)M_PI / 180.0f;
    const Vec3 target(0.0f, 0.5f, 0.0f);
    const Vec3 eye(target.x + gDist * std::cos(pitch) * std::sin(yaw),
                   target.y + gDist * std::sin(pitch),
                   target.z + gDist * std::cos(pitch) * std::cos(yaw));
    gluLookAt(eye.x, eye.y, eye.z, target.x, target.y, target.z, 0.0, 1.0, 0.0);

    // 바닥 격자 (y = 0)
    glDisable(GL_LIGHTING);
    glColor3f(0.30f, 0.31f, 0.35f);
    glBegin(GL_LINES);
    for (int k = -10; k <= 10; ++k) {
        glVertex3f(k * 0.1f, 0.0f, -1.0f); glVertex3f(k * 0.1f, 0.0f, 1.0f);
        glVertex3f(-1.0f, 0.0f, k * 0.1f); glVertex3f(1.0f, 0.0f, k * 0.1f);
    }
    glEnd();
    // 회전축
    glColor3f(0.90f, 0.35f, 0.30f);
    glBegin(GL_LINES); glVertex3f(0, 0, 0); glVertex3f(0, 1.2f, 0); glEnd();

    RebuildIfDirty();
    if (!gMesh.Empty()) {
        if (gShowSolid) {
            glEnable(GL_LIGHTING);
            SetupLighting();
            glEnable(GL_POLYGON_OFFSET_FILL);  // 와이어프레임이 면에 묻히지 않게 면을 살짝 뒤로
            glPolygonOffset(1.0f, 1.0f);
            glColor3f(0.72f, 0.56f, 0.38f);
            DrawMesh(gMesh);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_LIGHTING);
        }
        if (gShowWire) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            if (gShowSolid) glColor3f(0.15f, 0.12f, 0.10f);
            else            glColor3f(0.85f, 0.85f, 0.90f);
            DrawMesh(gMesh);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        if (gShowNormals) {
            glColor3f(0.30f, 0.90f, 0.40f);
            DrawMeshNormals(gMesh, 0.06f);
        }
    }

    // 안내
    glDisable(GL_DEPTH_TEST);
    BeginPixel2D(w, h);
    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(12, (float)h - 24, "3D PREVIEW (Surface of Revolution)", GLUT_BITMAP_HELVETICA_18);
    glColor3f(0.95f, 0.80f, 0.35f);
    DrawText(12, (float)h - 44, Fmt("slices %.0f  (rotate %.2f deg x %.0f)",
                                    gProfile.slices, 360.0 / gProfile.slices, gProfile.slices));
    DrawText(12, (float)h - 62, Fmt("vertices %.0f   triangles %.0f",
                                    (double)gMesh.vertices.size(), (double)gMesh.faces.size()));
    glColor3f(0.70f, 0.72f, 0.78f);
    DrawText(12, 70, "drag: rotate view   wheel: zoom   R: reset view");
    DrawText(12, 52, "+/-: slices   A: type angle   F: solid   W: wire   N: normals");
    DrawText(12, 34, "S: save (Models/*.dat + *.prof)   O: open *.prof   (type in console)");
    glColor3f(0.50f, 0.90f, 0.60f);
    DrawText(12, 14, gStatus);
}

void Display()
{
    glEnable(GL_SCISSOR_TEST);
    DrawEditor();
    DrawPreview();
    glDisable(GL_SCISSOR_TEST);
    glutSwapBuffers();
}

void Reshape(int w, int h)
{
    gWinW = std::max(w, 2);
    gWinH = std::max(h, 1);
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// 콘솔 입력 (이름·각도) — 입력하는 동안 창은 멈춘다
// ---------------------------------------------------------------------------
std::string Prompt(const char* message)
{
    std::cout << message << std::flush;
    std::string line;
    std::getline(std::cin, line);
    // 앞뒤 공백 제거
    const size_t s = line.find_first_not_of(" \t\r\n");
    const size_t e = line.find_last_not_of(" \t\r\n");
    return s == std::string::npos ? std::string() : line.substr(s, e - s + 1);
}

std::string StripExtension(std::string name)
{
    for (const char* ext : { ".dat", ".prof" }) {
        const size_t n = strlen(ext);
        if (name.size() > n && name.compare(name.size() - n, n, ext) == 0)
            name.erase(name.size() - n);
    }
    return name;
}

void InputAngle()
{
    std::cout << "\n[회전 각도] 콘솔에 입력하세요.\n";
    const std::string s = Prompt("회전 각도(도), 360의 약수 권장 (예: 30, 15, 10): ");
    const float angle = (float)atof(s.c_str());
    if (angle <= 0.0f || angle > 120.0f) {
        std::cout << "  → 취소 (0보다 크고 120 이하로 입력)\n";
        return;
    }
    const int slices = (int)std::lround(360.0f / angle);
    SetSlices(slices);
    std::cout << "  → 분할 수 " << gProfile.slices << " (실제 회전 각도 "
              << 360.0f / gProfile.slices << "도)\n";
    if (std::fabs(360.0f / angle - std::round(360.0f / angle)) > 1e-3f)
        std::cout << "  ※ 360의 약수가 아니어서 가장 가까운 각도로 맞췄습니다.\n";
    gStatus = "Angle set: " + Fmt("%.2f deg (%.0f slices)", 360.0 / gProfile.slices, gProfile.slices);
}

void SaveCurrent()
{
    RebuildIfDirty();
    if (gMesh.Empty()) {
        std::cout << "\n[저장] 점이 2개 이상 있어야 저장할 수 있습니다.\n";
        gStatus = "Save failed: need 2+ points";
        return;
    }
    std::cout << "\n[저장] 콘솔에 입력하세요.\n";
    const std::string name = StripExtension(Prompt("저장할 이름 (예: hermes_sole, 빈칸이면 취소): "));
    if (name.empty()) { std::cout << "  → 취소\n"; return; }

    _mkdir("Models");
    const std::string base = "Models/" + name;
    const bool ok = SaveMesh((base + ".dat").c_str(), gMesh) &&
                    SaveProfile((base + ".prof").c_str(), gProfile);
    if (ok) {
        std::cout << "  → 저장 완료: " << base << ".dat (정점 " << gMesh.vertices.size()
                  << ", 삼각형 " << gMesh.faces.size() << "), " << base << ".prof\n";
        gStatus = "Saved: " + base + ".dat / .prof";
    } else {
        std::cout << "  → 저장 실패: " << base << " (폴더·이름 확인)\n";
        gStatus = "Save failed";
    }
}

void OpenProfile()
{
    std::cout << "\n[열기] 콘솔에 입력하세요.\n";
    const std::string name = StripExtension(Prompt("열 이름 (Models/<이름>.prof, 빈칸이면 취소): "));
    if (name.empty()) { std::cout << "  → 취소\n"; return; }

    const std::string path = "Models/" + name + ".prof";
    Profile p;
    if (LoadProfile(path.c_str(), p)) {
        gProfile = p;
        gProfile.slices = std::clamp(gProfile.slices, kMinSlices, kMaxSlices);
        MarkDirty();
        std::cout << "  → 열기 완료: " << path << " (점 " << p.points.size() << "개)\n";
        gStatus = "Opened: " + path;
    } else {
        std::cout << "  → 열기 실패: " << path << "\n";
        gStatus = "Open failed: " + path;
    }
}

// ---------------------------------------------------------------------------
// 입력 콜백
// ---------------------------------------------------------------------------
void Mouse(int button, int state, int x, int y)
{
    const bool inEditor = x < HalfW();

    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        gDragIndex = -1;
        gRotating = false;
        glutPostRedisplay();
        return;
    }
    if (state != GLUT_DOWN) return;

    if (inEditor) {
        const Vec2 p = MouseToWorld(x, y);
        if (button == GLUT_LEFT_BUTTON) {
            const int hit = FindNearestPoint(p, kPickRadius);
            if (hit >= 0) {
                gDragIndex = hit;                         // 기존 점 잡기
            } else {
                gProfile.points.push_back(ClampToProfile(p));  // 새 점 추가 (끝에)
                gDragIndex = (int)gProfile.points.size() - 1;
            }
            MarkDirty();
        } else if (button == GLUT_RIGHT_BUTTON) {
            const int hit = FindNearestPoint(p, kPickRadius);
            if (hit >= 0) {
                gProfile.points.erase(gProfile.points.begin() + hit);
                MarkDirty();
            }
        }
    } else if (button == GLUT_LEFT_BUTTON) {
        gRotating = true;
        gLastX = x;
        gLastY = y;
    }
}

void Motion(int x, int y)
{
    if (gDragIndex >= 0 && gDragIndex < (int)gProfile.points.size()) {
        gProfile.points[gDragIndex] = ClampToProfile(MouseToWorld(std::min(x, HalfW() - 1), y));
        MarkDirty();
    } else if (gRotating) {
        gYaw   -= (x - gLastX) * 0.5f;
        gPitch += (y - gLastY) * 0.5f;
        gPitch  = std::clamp(gPitch, -85.0f, 85.0f);
        gLastX = x;
        gLastY = y;
        glutPostRedisplay();
    }
}

void MouseWheel(int /*wheel*/, int dir, int /*x*/, int /*y*/)
{
    gDist *= (dir > 0) ? 0.9f : 1.1f;
    gDist = std::clamp(gDist, 0.6f, 20.0f);
    glutPostRedisplay();
}

void Keyboard(unsigned char key, int, int)
{
    switch (key) {
    case '+': case '=': SetSlices(gProfile.slices + 1); break;
    case '-': case '_': SetSlices(gProfile.slices - 1); break;
    case 'a': case 'A': InputAngle(); break;
    case 'c': case 'C': gProfile.curve = !gProfile.curve; MarkDirty(); break;
    case 'f': case 'F': gShowSolid = !gShowSolid; break;
    case 'w': case 'W': gShowWire = !gShowWire; break;
    case 'n': case 'N': gShowNormals = !gShowNormals; break;
    case 'u': case 'U':
        if (!gProfile.points.empty()) { gProfile.points.pop_back(); MarkDirty(); }
        break;
    case 'x': case 'X': gProfile.points.clear(); MarkDirty(); break;
    case 'r': case 'R': gYaw = 35.0f; gPitch = 20.0f; gDist = 2.6f; break;
    case 's': case 'S': SaveCurrent(); break;
    case 'o': case 'O': OpenProfile(); break;
    case 27: glutLeaveMainLoop(); return;
    default: break;
    }
    glutPostRedisplay();
}

void PrintHelp()
{
    std::cout <<
        "==============================================\n"
        " SOR 모델러 : 프로파일을 Y축으로 회전해 3D Mesh 생성\n"
        "==============================================\n"
        " [왼쪽 화면] 2D 프로파일 (x = 반지름, y = 높이)\n"
        "   왼쪽 클릭: 점 추가 / 점 드래그: 이동 / 오른쪽 클릭: 삭제\n"
        "   U: 마지막 점 취소   X: 전부 지우기   C: 곡선(Catmull-Rom) 켜기/끄기\n"
        "   점 순서: 위에서 아래로 찍는 것을 권장 (0번이 맨 위)\n"
        "   빨간 선 = 회전축(Y축), 점선 = 높이 1 기준\n"
        " [오른쪽 화면] 3D 미리보기\n"
        "   드래그: 시점 회전   휠: 확대/축소   R: 시점 초기화\n"
        "   + / -: 분할 수 변경   A: 회전 각도 입력 (콘솔)\n"
        "   F: 면   W: 와이어프레임   N: 법선 표시\n"
        " [파일]\n"
        "   S: 저장 → Models/<이름>.dat (Mesh) + Models/<이름>.prof (프로파일)\n"
        "   O: Models/<이름>.prof 열어서 다시 편집\n"
        "   ※ S / O / A 를 누른 뒤에는 이 콘솔 창에 입력하세요.\n"
        " Esc: 종료\n"
        "==============================================\n";
}

// 처음 실행했을 때 보이는 예시 프로파일 (꽃병)
void LoadSampleProfile()
{
    gProfile.points = {
        { 0.26f, 1.00f }, { 0.20f, 0.92f }, { 0.17f, 0.78f }, { 0.28f, 0.55f },
        { 0.33f, 0.35f }, { 0.27f, 0.12f }, { 0.20f, 0.02f }, { 0.00f, 0.00f },
    };
    gProfile.slices = 24;
    gProfile.curve = true;
    gDirty = true;
}

} // namespace

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWinW, gWinH);
    glutCreateWindow("SOR Modeler");

    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutMouseWheelFunc(MouseWheel);
    glutKeyboardFunc(Keyboard);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    PrintHelp();
    LoadSampleProfile();
    glutMainLoop();
    return 0;
}
