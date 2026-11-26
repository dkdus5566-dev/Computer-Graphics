#include "Game.h"
#include <GL/freeglut.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

// ───────────── 능력 등록 ─────────────
// 함수 안의 static은 처음 쓰일 때 만들어지므로, 다른 .cpp의 전역 변수 초기화(RegisterAbility 호출)보다
// 늦게 만들어질 걱정이 없다.
static std::vector<Ability>& Registry() { static std::vector<Ability> r; return r; }
bool RegisterAbility(const Ability& a) { Registry().push_back(a); return true; }
const std::vector<Ability>& RegisteredAbilities() { return Registry(); }

namespace {
const float kPickRange = 1.4f;       // 아이템을 주울 수 있는 거리
// 단상: pedestal.dat는 분할 4 SOR — 아래 모서리 반지름 0.44, 위 모서리 0.31(높이 0.89), 윗면 가운데가 솟은 낮은 피라미드 (높이 약 1)
const float kPedHeight = 0.85f;     // 게임에서의 단상 높이 (y 크기)
const float kPedScale = 1.06f;      // 가로 크기 → 아래 한 변의 절반 = 0.44 × 1.06 / √2 ≈ 0.33
const float kPedHalf = 0.44f * kPedScale / 1.41421356f;
const float kBarsRange = 2.4f;       // 창살 칸 중심에서 이 거리 안이면 미노타우르스와 대화·전달 (앞 칸 가운데 = 2.0)

void Text(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(std::max(x, 10.0f), y);   // 시작점이 화면 밖이면 글자 전체가 안 그려지므로 왼쪽 끝에 맞춘다
    glutBitmapString(font, (const unsigned char*)s.c_str());
}
// 여러 줄('\n')을 가운데 정렬로 위에서 아래로
void TextCentered(float w, float y, const std::string& s)
{
    size_t start = 0;
    while (start <= s.size()) {
        size_t end = s.find('\n', start); if (end == std::string::npos) end = s.size();
        std::string line = s.substr(start, end - start);
        Text((w - glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)line.c_str())) / 2, y, line);
        y -= 24; start = end + 1;
    }
}
float TextWidth(const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18)
{
    return (float)glutBitmapLength(font, (const unsigned char*)s.c_str());
}
}

bool Game::Init()
{
    if (!maze.Load("Maps/maze.txt")) return false;
    maze.Build();
    for (Item& it : items) it.model.Load(it.file);   // 없으면 임시 구로 그린다
    if (!LoadMesh("Models/pedestal.dat", pedestal)) printf("단상 모델 없음: Models/pedestal.dat (단상 없이 진행)\n");
    Reset();
    return true;
}

void Game::Reset()
{
    maze.exitOpen = maze.hiddenOpen = maze.pitsPassable = false;
    viewDistance = 7.0f;
    xray = false;
    metMinotaur = false;
    player = Player();
    int c, r;
    if (maze.Find('S', c, r)) {
        maze.CellCenter(c, r, player.x, player.z);
        // 시작할 때 막히지 않은 쪽을 바라본다 (동 → 남 → 서 → 북)
        const int dc[4] = { 1, 0, -1, 0 }, dr[4] = { 0, 1, 0, -1 }; const float yaw[4] = { 90, 180, 270, 0 };
        for (int k = 0; k < 4; ++k) if (!maze.IsSolid(c + dc[k], r + dr[k])) { player.yaw = yaw[k]; break; }
    }
    safeX = player.x; safeZ = player.z;
    hasMinotaur = maze.Find('M', c, r);
    if (hasMinotaur) maze.CellCenter(c, r, minoX, minoZ);
    if (maze.Find('B', c, r)) maze.CellCenter(c, r, barsX, barsZ); else { barsX = minoX; barsZ = minoZ; }
    for (Item& it : items) {
        it.onMap = maze.Find(it.letter, c, r);
        if (it.onMap) maze.CellCenter(c, r, it.x, it.z);
        it.carried = it.delivered = false;
    }
    time = 0;
    message.clear();
}

World Game::MakeWorld(float dt) { return World{ *this, player, maze, viewDistance, xray, time, dt }; }

bool Game::NearBars() const { return hasMinotaur && std::hypot(player.x - barsX, player.z - barsZ) < kBarsRange; }

bool Game::ItemPosition(char item, float& x, float& z) const
{
    for (const Item& it : items) if (it.letter == item && it.onMap) { x = it.x; z = it.z; return true; }
    return false;
}
bool Game::IsCarried(char item) const   { for (const Item& it : items) if (it.letter == item) return it.carried; return false; }
bool Game::IsDelivered(char item) const { for (const Item& it : items) if (it.letter == item) return it.delivered; return false; }
void Game::ShowMessage(const std::string& text, float seconds) { message = text; messageTime = seconds; }

// ───────────── 입력 ─────────────
void Game::KeyDown(unsigned char key)
{
    key = (unsigned char)std::tolower(key);
    keys[key] = true;
    if (state == State::Title) {
        if (key == 13 || key == ' ') { Reset(); state = State::Play; mouseCentered = false; glutSetCursor(GLUT_CURSOR_NONE); }
        if (key == 27) quit = true;
        return;
    }
    if (state == State::Clear) {
        if (key == 13 || key == 27) { state = State::Title; glutSetCursor(GLUT_CURSOR_INHERIT); }
        return;
    }
    if (key == 27) { state = State::Title; glutSetCursor(GLUT_CURSOR_INHERIT); return; }
    if (key == 'e') Interact();
    World w = MakeWorld(0);
    for (const Ability& a : RegisteredAbilities()) if (a.onKey && IsDelivered(a.item)) a.onKey(w, key);
}

void Game::KeyUp(unsigned char key) { keys[(unsigned char)std::tolower(key)] = false; }

void Game::SpecialDown(int key)
{
    if (key >= 0 && key < 512) special[key] = true;
    if (state != State::Play) return;
    World w = MakeWorld(0);
    for (const Ability& a : RegisteredAbilities()) if (a.onSpecial && IsDelivered(a.item)) a.onSpecial(w, key);
}

void Game::SpecialUp(int key) { if (key >= 0 && key < 512) special[key] = false; }

// 마우스 시점: 커서를 화면 가운데로 되돌리면서, 가운데에서 움직인 만큼 회전한다
void Game::MouseMove(int x, int y, int w, int h)
{
    if (state != State::Play) return;
    const int cx = w / 2, cy = h / 2;
    if (!mouseCentered) { mouseCentered = true; glutWarpPointer(cx, cy); return; }
    const int dx = x - cx, dy = y - cy;
    if (dx == 0 && dy == 0) return;
    player.yaw += dx * player.mouseSensitivity;
    player.pitch = std::clamp(player.pitch - dy * player.mouseSensitivity, -60.0f, 60.0f);
    glutWarpPointer(cx, cy);
}

void Game::Interact()
{
    // 1) 창살 앞: 처음에는 대화, 그다음부터 들고 있는 아이템을 모두 전달 → 고쳐 받고 능력 해금
    if (NearBars()) {
        if (!metMinotaur) {
            metMinotaur = true;
            ShowMessage("\"I am no monster, only a prisoner of this labyrinth.\n"
                        "The relics hidden here are broken. Bring them to me and I will mend them.\n"
                        "Their power will open the ways that are closed to you.\"", 7.0f);
            return;
        }
        bool any = false;
        World w = MakeWorld(0);
        for (Item& it : items) {
            if (!it.carried) continue;
            it.carried = false; it.delivered = true; any = true;
            // 기본 메시지를 먼저 띄우고, 능력 코드(onUnlock)가 자기 메시지로 바꿀 수 있게 한다
            ShowMessage(std::string("The Minotaur mends the ") + it.name + ". A new power awakens.");
            if (it.letter == 'T') { maze.exitOpen = true; ShowMessage("Ariadne's Thread shows the way. The exit is open!"); }
            for (const Ability& a : RegisteredAbilities()) if (a.item == it.letter && a.onUnlock) a.onUnlock(w);
        }
        if (!any) ShowMessage("\"Bring me the relics. I will mend them.\"");
        return;
    }
    // 2) 가까운 아이템 줍기
    for (Item& it : items) {
        if (!it.onMap || it.carried || it.delivered) continue;
        if (std::hypot(player.x - it.x, player.z - it.z) < kPickRange) {
            it.carried = true;
            ShowMessage(std::string("You picked up the ") + it.name + ". Bring it to the Minotaur.");
            return;
        }
    }
}

// ───────────── 갱신 ─────────────
void Game::Update(float dt)
{
    lastDt = dt;
    if (messageTime > 0) messageTime -= dt;
    if (state != State::Play) return;
    time += dt;

    // 회전: ← →
    if (special[GLUT_KEY_LEFT])  player.yaw -= player.turnSpeed * dt;
    if (special[GLUT_KEY_RIGHT]) player.yaw += player.turnSpeed * dt;

    // 이동: W S / ↑ ↓ 앞뒤, A D 옆걸음. 앞쪽 벡터 f = (sin yaw, −cos yaw), 오른쪽 = (cos yaw, sin yaw)
    float f = 0, s = 0;
    if (keys['w'] || special[GLUT_KEY_UP])   f += 1;
    if (keys['s'] || special[GLUT_KEY_DOWN]) f -= 1;
    if (keys['d']) s += 1;
    if (keys['a']) s -= 1;
    if (f != 0 || s != 0) {
        float fx, fz; player.Forward(fx, fz);
        float mx = fx * f - fz * s, mz = fz * f + fx * s;
        const float len = std::sqrt(mx * mx + mz * mz);
        const float step = player.speed * player.speedMultiplier * dt / len;
        player.Move(maze, mx * step, mz * step);
    }

    World w = MakeWorld(dt);
    for (const Ability& a : RegisteredAbilities()) if (a.update && IsDelivered(a.item)) a.update(w);
    maze.hiddenOpen = xray;
    PushOutOfPedestals();
    KeepSafe();

    // 출구 칸에 들어가면 탈출
    int c, r, pc, pr;
    maze.CellOf(player.x, player.z, pc, pr);
    if (maze.exitOpen && maze.Find('X', c, r) && c == pc && r == pr) {
        state = State::Clear;
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }
}

// 가짜 벽 투시가 끝나거나 대쉬가 구덩이 위에서 끝나 '지나갈 수 없는 칸'과 겹치면,
// 마지막으로 안전했던 위치로 되돌린다. 안전한 위치는 투시·대쉬가 없어도 서 있을 수 있는 곳만 기억한다.
void Game::KeepSafe()
{
    const bool hidden = maze.hiddenOpen, pits = maze.pitsPassable;
    maze.hiddenOpen = maze.pitsPassable = false;
    const bool strictOk = !maze.Collides(player.x, player.z, player.radius);
    maze.hiddenOpen = hidden; maze.pitsPassable = pits;
    if (strictOk) { safeX = player.x; safeZ = player.z; return; }
    if (maze.Collides(player.x, player.z, player.radius)) {
        ShowMessage(maze.InPit(player.x, player.z) ? "The spikes! You stagger back." : "The illusion closes around you. You stumble back.");
        player.x = safeX; player.z = safeZ;
    }
}

// ───────────── 그리기 ─────────────
void Game::DrawItem(const Item& it, bool transparent) const
{
    if (!it.onMap || it.carried || it.delivered) return;
    glPushMatrix();
    // 떠 있으면서 천천히 돈다 → "주울 수 있는 물건" 표시
    glTranslatef(it.x, kPedHeight + 0.05f + 0.03f * std::sin(time * 2.0f + it.x), it.z);   // 단상 바로 위에 살짝 떠 있다 (0.02~0.08)
    glRotatef(time * 40.0f, 0, 1, 0);
    if (it.model.Loaded()) {
        // 조립 모델의 높이를 it.height로 맞추고, 바닥 중심이 원점에 오게 옮긴다
        const Vec3& a = it.model.boundsMin; const Vec3& b = it.model.boundsMax;
        const float k = it.height / std::max(b.y - a.y, 1e-4f);
        glScalef(k, k, k);
        glTranslatef(-(a.x + b.x) / 2, -a.y, -(a.z + b.z) / 2);
        if (transparent) it.model.DrawTransparent(); else it.model.DrawOpaque();
    } else if (!transparent) {
        // 모델이 아직 없으면 임시 구
        GLfloat e[] = { it.color[0] * 0.5f, it.color[1] * 0.5f, it.color[2] * 0.5f, 1 }, zero[] = { 0, 0, 0, 1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
        glColor3fv(it.color);
        glTranslatef(0, 0.25f, 0);
        glutSolidSphere(0.22, 24, 16);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    }
    glPopMatrix();
}

// 단상: 분할 4인 SOR이라 정점 법선을 그대로 쓰면 모서리가 둥글게 보인다.
// 각진 돌기둥처럼 보이도록 삼각형마다 면 법선(두 변의 외적)을 계산해 쓴다 (flat shading).
void Game::DrawPedestal(float x, float z) const
{
    if (pedestal.Empty()) return;
    glPushMatrix();
    glTranslatef(x, 0, z);
    glRotatef(45, 0, 1, 0);                        // 분할 4의 첫 꼭짓점이 +x에 있으므로 45° 돌려 면을 벽과 나란히
    glScalef(kPedScale, kPedHeight, kPedScale);
    GLfloat sp[] = { 0.25f, 0.24f, 0.22f, 1 }, zero[] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sp); glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20);
    // 돌 텍스처: .dat에는 텍스처 좌표가 없으므로, 면의 방향에 따라 정점 위치를 평면에 투영해 좌표로 쓴다 (박스 매핑)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, maze.StoneTexture());
    glBegin(GL_TRIANGLES);
    for (const Face& f : pedestal.faces) {
        const Vec3& a = pedestal.vertices[f.v[0]]; const Vec3& b = pedestal.vertices[f.v[1]]; const Vec3& c = pedestal.vertices[f.v[2]];
        const Vec3 n = Normalize(Cross(b - a, c - a));
        if (n.y > 0.9f) glColor3f(1.0f, 0.95f, 0.85f); else glColor3f(0.92f, 0.85f, 0.74f);   // 윗면은 조금 밝게
        glNormal3f(n.x, n.y, n.z);
        for (const Vec3* v : { &a, &b, &c }) {
            if (std::fabs(n.y) > 0.9f)                 glTexCoord2f(v->x * 0.8f, v->z * 0.8f);   // 윗면·아랫면: xz 평면
            else if (std::fabs(n.x) > std::fabs(n.z))  glTexCoord2f(v->z * 0.8f, v->y * 0.8f);   // x를 향한 옆면: zy 평면
            else                                       glTexCoord2f(v->x * 0.8f, v->y * 0.8f);   // z를 향한 옆면: xy 평면
            glVertex3f(v->x, v->y, v->z);
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
    glPopMatrix();
}

// 플레이어 원과 단상 아래 사각형(한 변의 절반 kPedHalf)의 충돌: 사각형에서 원 중심에 가장 가까운 점까지가
// 반지름보다 가까우면 그 방향으로 밀어낸다 (벽 충돌과 같은 원-상자 방식)
void Game::PushOutOfPedestals()
{
    if (pedestal.Empty()) return;
    for (const Item& it : items) {
        if (!it.onMap) continue;
        const float nx = std::clamp(player.x, it.x - kPedHalf, it.x + kPedHalf);
        const float nz = std::clamp(player.z, it.z - kPedHalf, it.z + kPedHalf);
        float dx = player.x - nx, dz = player.z - nz, d = std::sqrt(dx * dx + dz * dz);
        if (d >= player.radius) continue;
        if (d < 1e-4f) { dx = player.x - it.x; dz = player.z - it.z; d = std::max(std::hypot(dx, dz), 1e-4f); }   // 중심이 안에 들어간 경우
        player.Move(maze, dx / d * (player.radius - d + 1e-3f), dz / d * (player.radius - d + 1e-3f));
    }
}

// 임시 미노타우르스: 미노타우르스 SOR 모델이 생기면 .item으로 바꾼다
void Game::DrawMinotaur() const
{
    if (!hasMinotaur) return;
    glPushMatrix();
    glTranslatef(minoX, 0, minoZ);
    glRotatef(std::atan2(player.x - minoX, player.z - minoZ) * 180.0f / (float)M_PI, 0, 1, 0);   // 플레이어를 바라본다
    GLUquadric* q = gluNewQuadric();
    glColor3f(0.30f, 0.20f, 0.14f);
    glPushMatrix(); glRotatef(-90, 1, 0, 0); gluCylinder(q, 0.45, 0.35, 1.5, 24, 4); glPopMatrix();   // 몸통
    glPushMatrix(); glTranslatef(0, 1.75f, 0.05f); glScalef(1, 0.9f, 1.15f); gluSphere(q, 0.32, 24, 16); glPopMatrix();   // 머리
    glColor3f(0.85f, 0.80f, 0.65f);
    for (int s = -1; s <= 1; s += 2) {   // 뿔
        glPushMatrix(); glTranslatef(0.25f * s, 1.9f, 0); glRotatef(70.0f * s, 0, 0, 1); glRotatef(-90, 1, 0, 0);
        gluCylinder(q, 0.07, 0.0, 0.45, 12, 2); glPopMatrix();
    }
    GLfloat e[] = { 0.9f, 0.3f, 0.1f, 1 }, zero[] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e); glColor3f(1, 0.4f, 0.1f);
    for (int s = -1; s <= 1; s += 2) { glPushMatrix(); glTranslatef(0.12f * s, 1.82f, 0.32f); gluSphere(q, 0.04, 8, 6); glPopMatrix(); }   // 눈
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    gluDeleteQuadric(q);
    glPopMatrix();
}

void Game::Draw(int w, int h)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (state == State::Play) {
        // Projection: 원근 투영
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(player.fov, (double)w / std::max(h, 1), 0.05, 80.0);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();

        // 플레이어가 든 불빛: 카메라를 곱하기 전에 (0,0,0)에 두면 눈 좌표계 원점 = 항상 플레이어 위치
        GLfloat lp[] = { 0, 0, 0, 1 }, ld[] = { 1.0f, 0.86f, 0.66f, 1 }, ls[] = { 0.6f, 0.55f, 0.45f, 1 };
        glLightfv(GL_LIGHT0, GL_POSITION, lp); glLightfv(GL_LIGHT0, GL_DIFFUSE, ld); glLightfv(GL_LIGHT0, GL_SPECULAR, ls);
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.9f / viewDistance);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.6f / (viewDistance * viewDistance));

        // 가시거리: 선형 안개. viewDistance 너머는 완전히 어둡다
        GLfloat fog[] = { 0, 0, 0, 1 };
        glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_LINEAR); glFogfv(GL_FOG_COLOR, fog);
        glFogf(GL_FOG_START, viewDistance * 0.5f); glFogf(GL_FOG_END, viewDistance);   // 가시거리의 절반까지는 안개 없음

        // View: 카메라
        player.ApplyCamera();

        // 미노타우르스 방의 횃불: 카메라를 곱한 뒤에 월드 좌표로 두면 방 안에 고정된다
        if (hasMinotaur) {
            const float flicker = 0.9f + 0.1f * std::sin(time * 11.0f) * std::sin(time * 7.3f);
            GLfloat tp[] = { minoX + 0.7f, 2.0f, minoZ + 0.7f, 1 }, td[] = { 1.0f * flicker, 0.6f * flicker, 0.3f * flicker, 1 };
            glEnable(GL_LIGHT1);
            glLightfv(GL_LIGHT1, GL_POSITION, tp); glLightfv(GL_LIGHT1, GL_DIFFUSE, td);
            glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.5f);
            glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.08f);
        }

        // 1) 불투명한 것 → 2) 반투명한 것 (깊이 버퍼를 먼저 채워야 투명한 것 뒤가 올바르게 가려진다)
        maze.Draw(time);
        for (const Item& it : items) if (it.onMap) DrawPedestal(it.x, it.z);   // 아이템을 가져가도 단상은 남는다
        for (const Item& it : items) DrawItem(it, false);
        DrawMinotaur();
        World wd = MakeWorld(lastDt);
        for (const Ability& a : RegisteredAbilities()) if (a.drawWorld && IsDelivered(a.item)) a.drawWorld(wd);
        for (const Item& it : items) DrawItem(it, true);
        maze.DrawTransparent(time);
        if (xray) {
            // 투시 중: 남은 아이템이 벽 너머로 빛난다 (깊이 검사를 끄고 더하기 혼합)
            glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_FOG);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            for (const Item& it : items) {
                if (!it.onMap || it.carried || it.delivered) continue;
                for (int k = 0; k < 3; ++k) {
                    glColor4f(it.color[0], it.color[1], it.color[2], 0.35f - 0.1f * k);
                    glPushMatrix(); glTranslatef(it.x, kPedHeight + 0.35f, it.z);
                    glutSolidSphere(0.25 + 0.2 * k + 0.05 * std::sin(time * 4.0f), 16, 12);
                    glPopMatrix();
                }
            }
            glDisable(GL_BLEND); glEnable(GL_FOG); glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
        }
        glDisable(GL_FOG);
    }
    DrawHud(w, h);
}

void Game::DrawHud(int w, int h)
{
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);

    if (state == State::Title) {
        const std::string t1 = "THE LABYRINTH OF THE MINOTAUR", t2 = "Press Enter to begin   /   Esc to quit";
        const std::string t3 = "Find the four relics and bring them to the Minotaur. Ariadne's Thread opens the exit.";
        glColor3f(0.95f, 0.78f, 0.40f); Text((w - TextWidth(t1, GLUT_BITMAP_TIMES_ROMAN_24)) / 2, h * 0.6f, t1, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.85f, 0.85f, 0.85f); Text((w - TextWidth(t3)) / 2, h * 0.5f, t3);
        glColor3f(1, 1, 1); Text((w - TextWidth(t2)) / 2, h * 0.4f, t2);
        Text(20, 20, "W A S D / arrows: move   mouse: look   E: pick up / give   Esc: menu", GLUT_BITMAP_HELVETICA_12);
    } else if (state == State::Clear) {
        const std::string t1 = "YOU ESCAPED THE LABYRINTH", t2 = "Press Enter to return to the title";
        glColor3f(1.0f, 0.9f, 0.5f); Text((w - TextWidth(t1, GLUT_BITMAP_TIMES_ROMAN_24)) / 2, h * 0.55f, t1, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1, 1, 1); Text((w - TextWidth(t2)) / 2, h * 0.45f, t2);
    } else {
        // 조준점
        glColor3f(1, 1, 1);
        glBegin(GL_LINES); glVertex2f(w / 2.f - 7, h / 2.f); glVertex2f(w / 2.f + 7, h / 2.f); glVertex2f(w / 2.f, h / 2.f - 7); glVertex2f(w / 2.f, h / 2.f + 7); glEnd();
        // 들고 있는 것 / 해금된 능력
        std::string carry = "Carrying:", power = "Powers:";
        for (const Item& it : items) { if (it.carried) carry += std::string(" ") + it.name + ","; if (it.delivered) power += std::string(" ") + it.name + ","; }
        if (carry.back() == ',') carry.pop_back(); else carry += " -";
        if (power.back() == ',') power.pop_back(); else power += " -";
        glColor3f(0.95f, 0.85f, 0.6f); Text(20, h - 30.f, carry); Text(20, h - 55.f, power);
        // 상호작용 안내
        std::string hint;
        if (NearBars()) hint = metMinotaur ? "[E] Give relics to the Minotaur" : "[E] Talk to the Minotaur";
        for (const Item& it : items)
            if (it.onMap && !it.carried && !it.delivered && std::hypot(player.x - it.x, player.z - it.z) < kPickRange) hint = std::string("[E] Pick up the ") + it.name;
        if (!hint.empty()) { glColor3f(1, 1, 1); Text((w - TextWidth(hint)) / 2, h * 0.3f, hint); }
        if (messageTime > 0) { glColor3f(1.0f, 0.9f, 0.55f); TextCentered((float)w, h * 0.22f, message); }
        World wd = MakeWorld(lastDt);
        for (const Ability& a : RegisteredAbilities()) if (a.drawHud && IsDelivered(a.item)) a.drawHud(wd, w, h);
    }
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}
