// 미노타우르스의 미궁 — 3D 미로 게임 시작점 (창 만들기, OpenGL 기본 상태, GLUT 콜백 연결)
#include <GL/freeglut.h>
#include <algorithm>
#include <cstdio>
#include "ConsoleUtf8.h"
#include "Game.h"

static Game game;
static int winW = 1280, winH = 720;
static int lastTime = 0;

static void Display() { game.Draw(winW, winH); glutSwapBuffers(); }
static void Reshape(int w, int h) { winW = std::max(w, 1); winH = std::max(h, 1); glViewport(0, 0, winW, winH); }

// 매 프레임: 지난 시간(dt)만큼 게임을 진행하고 다시 그린다
static void Idle()
{
    const int now = glutGet(GLUT_ELAPSED_TIME);
    const float dt = std::min((now - lastTime) / 1000.0f, 0.05f);   // 창을 끌 때 등 너무 큰 dt는 자른다
    lastTime = now;
    game.Update(dt);
    if (game.WantsQuit()) { glutLeaveMainLoop(); return; }
    glutPostRedisplay();
}

static void Keyboard(unsigned char k, int, int)   { game.KeyDown(k); if (game.WantsQuit()) glutLeaveMainLoop(); }
static void KeyboardUp(unsigned char k, int, int) { game.KeyUp(k); }
static void Special(int k, int, int)              { game.SpecialDown(k); }
static void SpecialUp(int k, int, int)            { game.SpecialUp(k); }
static void Motion(int x, int y)                  { game.MouseMove(x, y, winW, winH); }

int main(int argc, char** argv)
{
    InitConsoleUtf8();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("The Labyrinth of the Minotaur");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    // 조명·재질 기본 상태: glColor가 재질의 ambient·diffuse가 된다
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);   // 크기 변환(glScalef) 뒤에도 법선 길이를 1로
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    GLfloat ambient[] = { 0.16f, 0.15f, 0.14f, 1 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glShadeModel(GL_SMOOTH);

    if (!game.Init()) {
        printf("게임을 시작할 수 없습니다. 저장소 폴더에서 실행했는지 확인하세요 (Maps/, Models/ 필요).\n");
        return 1;
    }

    glutIgnoreKeyRepeat(1);   // 키를 누르고 있을 때 반복 입력 대신 누름/뗌만 받는다
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutIdleFunc(Idle);
    glutKeyboardFunc(Keyboard);
    glutKeyboardUpFunc(KeyboardUp);
    glutSpecialFunc(Special);
    glutSpecialUpFunc(SpecialUp);
    glutPassiveMotionFunc(Motion);
    glutMotionFunc(Motion);
    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutMainLoop();
    return 0;
}
