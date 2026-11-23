# 팀 작업 규칙 (CONTRIBUTING)

> **AI(Claude, ChatGPT, Copilot 등)로 작업할 때는 이 파일을 먼저 읽히고 시작한다.**
> 사람과 AI 모두 아래 규칙을 따른다. 규칙을 바꾸고 싶으면 PR로 제안한다.

프로젝트: **OpenGL 기반 SOR 모델러 + 3D 미로 게임** (컴퓨터그래픽스 기말 프로젝트)
주제: 미노타우르스와 미궁 — 아이템(SOR 모델)을 미노타우르스에게 가져가면 능력을 얻는다.

---

## 1. 빌드 환경

| 항목 | 값 |
|---|---|
| IDE | Visual Studio 2026 (툴셋 **v145**), "C++를 사용한 데스크톱 개발" + Windows SDK |
| 플랫폼 | **Debug \| x86 (Win32) 전용** — freeglut.dll이 x86용뿐 |
| 그래픽스 | OpenGL (고정 파이프라인) + freeglut 3.2.1 (`freeglut/`, 저장소에 포함) |
| 언어 | C++17 |
| 솔루션 | `CG_Final.slnx` |

- 빌드하면 exe가 `Bin/Debug/`에 생기고 `freeglut.dll`이 자동 복사된다.
- 디버그 실행 시 작업 폴더 = **저장소 루트**. 파일 경로는 항상 상대경로 (`"Models/xxx.dat"`). 절대경로(`C:\...`) 금지.

---

## 2. 폴더 구조와 담당

```
CG_Final.slnx
Shared.props          ← 공통 빌드 설정 (수정은 팀 합의 후 PR로)
freeglut/             ← 수정 금지
Common/               ← 모델러·게임 공용 코드 (Mesh, 파일 입출력, SOR 생성)
SORModeler/           ← SOR 모델러 (프로파일 → 3D Mesh → .dat 저장)
MazeGame/             ← 미로 게임
Models/               ← SOR 모델 데이터 (.dat 등)
```

| 아이템 (SOR) | 능력 | 게임 코드 파일 (예시) | 모델 파일 이름 |
|---|---|---|---|
| 등불 | 가시거리 확보 | `MazeGame/Lamp.cpp` | `Models/lamp_*.dat` |
| 헤르메스의 신발 | 대쉬 | `MazeGame/HermesShoes.cpp` | `Models/hermes_*.dat` |
| 아르고스의 눈 | 텔레파시 (실타래 발광) | `MazeGame/ArgosEye.cpp` | `Models/argos_*.dat` |
| 아리아드네의 실타래 | 탈출 | `MazeGame/AriadneThread.cpp` | `Models/ariadne_*.dat` |

- **SOR 모델러는 하나**를 같이 쓴다. 각자 맡은 SOR은 모델러로 만든 **데이터 파일**이다.
- 미로·플레이어 이동·충돌·카메라 기반 담당: **(팀에서 정하기)**

---

## 3. 소스 파일 추가 (중요)

**폴더에 `.cpp`/`.h` 파일을 넣기만 하면 자동으로 빌드에 포함된다.** (`Shared.props`)

| 폴더 | 포함되는 프로젝트 |
|---|---|
| `Common/` | SORModeler + MazeGame 둘 다 |
| `SORModeler/` (하위 폴더 포함) | SORModeler만 |
| `MazeGame/` (하위 폴더 포함) | MazeGame만 |

- ❌ Visual Studio의 **"추가 > 새 항목"을 쓰지 않는다.** `.vcxproj`에 줄이 추가돼 같은 파일이 두 번 들어가고, 팀원끼리 merge 충돌이 난다.
- ✅ 탐색기/VS Code에서 파일을 만들거나, VS 솔루션 탐색기 위쪽 **"모든 파일 표시"**에서 폴더에 만든다. 새 파일이 안 보이면 솔루션을 다시 연다.
- `.vcxproj`, `CG_Final.slnx`, `Shared.props`는 **팀 합의 후에만** 고친다.
- 소스 인코딩: **UTF-8 (BOM)** — `.editorconfig`가 VS에서 자동 적용. 한글 주석 OK.

---

## 4. Git 작업 순서 (GitHub Flow)

**main은 항상 빌드되는 상태. main에 직접 push 금지. 모든 작업은 브랜치 → PR → 팀원 1명 확인 → merge.**

```
git switch main
git pull                                  # ① 최신 main 받기
git switch -c feature/hermes-dash         # ② 내 브랜치
# ... 작업, 빌드 확인 ...
git add <파일>
git commit -m "[Game] 대쉬 이동 추가"      # ③ 작게 자주 커밋
git push -u origin feature/hermes-dash    # ④ 브랜치 push
# ⑤ GitHub에서 Pull Request 만들기 → 팀원 확인 → Merge
git switch main && git pull               # ⑥ merge 후 최신화
git branch -d feature/hermes-dash         #    끝난 브랜치 삭제
```

### 브랜치 이름
- 기능: `feature/<내용>` (예: `feature/sor-modeler`, `feature/hermes-dash`, `feature/maze-collision`)
- 버그: `fix/<내용>` · 설정/문서: `setup/<내용>`
- **담당이 드러나게** 짓는다 (`feature/test` 같은 이름 금지).

### 커밋 메시지
`[영역] 한 일` — 영역: `Setup` `Common` `SOR` `Game` `Model` `Docs`
- `[SOR] 프로파일 점 드래그 편집`
- `[Game] 벽 충돌 처리`
- `[Model] hermes_sole.dat 추가`

### 하지 말 것
- ❌ main에 직접 push, `git push --force`
- ❌ 빌드 결과물(`Bin/`, `Build/`, `.vs/`, exe, obj) 커밋 — `.gitignore`가 막음
- ❌ 남의 담당 파일을 말없이 수정 → PR 댓글/단톡으로 먼저 말하기
- ❌ 개인정보, 개인 메모(CLAUDE.md 등) 커밋
- ❌ 며칠씩 커밋 없이 쌓아 두기 → 충돌 커짐. 작업 중에도 `git merge main`으로 자주 최신화

### PR 올릴 때
- 무엇을 했는지 / 어떻게 확인했는지(빌드·실행) / AI를 썼다면 무엇을 시켰는지 적는다.
- **Debug | x86 빌드가 되는 상태**로만 PR을 올린다.

---

## 5. 모델 파일 형식 · 좌표 규칙 (⚠️ 제안 — 팀 회의에서 확정)

### `.dat` (텍스트, Mesh 1개)
```
# 주석
VERTEX = n
x y z            ← n줄 (회전된 정점)
NORMAL = n
nx ny nz         ← n줄 (정점 normal, 단위 벡터)
FACE = m
i0 i1 i2         ← m줄 (삼각형 index, 0부터)
```
- 저장·불러오기 코드는 `Common/`에 두고 **모델러와 게임이 같은 코드**를 쓴다.

### 좌표 규칙
- 회전축: **Y축**, 프로파일은 x ≥ 0 (반지름), y = 높이
- 바닥: **y = 0**, 크기: 높이 **≈ 1** (게임에서 scale로 조절)
- 삼각형: 바깥에서 볼 때 **반시계(CCW)**
- 부품 여러 개짜리 아이템: 부품별 `.dat` + 조립 정보(부품 파일, 위치·회전·크기·색) — 형식 회의에서 확정

---

## 6. AI 사용 규칙

- 과제상 **생성형 AI 사용은 허용**, 단 보고서에 **AI 활용 및 문제 해결 요약**이 필수 (평가 10점).
- 각자 **AI 사용 내역을 기록**한다: 날짜 / 담당 / 사용한 AI / AI가 한 일 / 내가 검증·수정한 것.
- AI가 만든 코드도 **내가 설명할 수 있어야** 한다 (시연은 개별 구술 평가).
- AI에게 시킬 때: 이 파일을 먼저 읽히고, **자기 담당 폴더·파일만** 고치게 하고, main에 직접 push하지 않게 한다.

---

## 7. 제출 (2026-12-09 자정 / 시연 12-10)

- `Report.pdf`: 표지·목차 제외 10장 이내, **AI 활용 및 문제 해결 요약 / 팀원별 담당 기능** 필수
- `Program.zip`: `Source/`(cpp, h) + `Bin/`(exe + dll + Models) — Debug/Release 폴더 통째로 넣으면 감점
