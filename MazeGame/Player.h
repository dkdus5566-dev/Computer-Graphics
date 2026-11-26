#pragma once
// 1인칭 플레이어: 위치, 시선 방향, 이동과 벽 충돌, 카메라(View 행렬)
class Maze;

class Player {
public:
    float x = 0, z = 0;             // 바닥 위 위치 (y는 eyeHeight로 고정)
    float yaw = 90, pitch = 0;      // 도(degree). yaw 0 = −z 방향, 90 = +x 방향. pitch 위쪽이 +
    float eyeHeight = 1.15f;
    float radius = 0.30f;           // 위에서 본 몸통 원의 반지름 (충돌용)
    float speed = 3.4f;             // 초당 이동 거리
    float mouseSensitivity = 0.12f; // 마우스 1픽셀당 회전 각도 (도)
    float speedMultiplier = 1.0f;   // 능력(대쉬 등)이 바꾸는 배율
    float turnSpeed = 110.0f;       // 방향키 회전 속도 (도/초)
    float fov = 70.0f;              // 세로 시야각 (Projection)

    // 앞쪽 방향 (바닥 평면, 단위 벡터)
    void Forward(float& dx, float& dz) const;

    // (dx, dz)만큼 이동. 한 번에 멀리 가면 벽을 뚫을 수 있으므로 작은 단계로 나누고,
    // 단계마다 x와 z를 따로 검사해 막힌 축만 멈춘다 → 벽을 따라 미끄러진다.
    void Move(const Maze& maze, float dx, float dz);

    // 현재 모델뷰 행렬에 카메라 변환(gluLookAt)을 곱한다
    void ApplyCamera() const;
};
