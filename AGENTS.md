# AI 에이전트 안내 (Codex, Cursor, Claude 등 공통)

이 저장소는 4인 팀의 컴퓨터그래픽스 기말 프로젝트(OpenGL SOR 모델러 + 3D 미로 게임)다.

**작업을 시작하기 전에 반드시 [`CONTRIBUTING.md`](CONTRIBUTING.md)를 끝까지 읽고 그대로 따른다.**
처음 시작하는 사용자라면 `CONTRIBUTING.md`의 **"0. 처음 시작하기"** 순서대로 진행한다.

## 작업을 시작할 때마다 (이미 작업 중인 사람도)

1. `git fetch` 후 `origin/main`의 `AGENTS.md`, `CONTRIBUTING.md`가 바뀌었으면 사용자에게 알리고, 확인을 받아 내 브랜치에 `origin/main`을 merge한 뒤 **다시 읽는다.** 커밋하지 않은 변경이 있으면 먼저 알린다.
2. `AI_LOG.local.md`가 없으면 만들고, 이 대화에서 지금까지 한 일을 정리해 적는다. 사용자가 한 일은 추측하지 않고 묻는다.
3. 이 저장소의 `git user.email`이 GitHub noreply 주소인지 확인한다. 아니면 커밋 전에 사용자에게 알린다.
4. commit, push, PR, merge는 할 때마다 먼저 사용자에게 묻는다.
5. 사용자가 이전에 준 지시와 이 규칙이 다르면 사용자에게 묻는다.

## 꼭 지킬 것 (요약 — 자세한 내용은 CONTRIBUTING.md)

1. **main에 직접 commit/push 하지 않는다.** 항상 `feature/<내용>` 브랜치에서 작업하고 PR로 올린다. `git push --force` 금지.
2. **사용자가 맡은 담당 파일만** 만들고 고친다. 담당 파일 이름은 CONTRIBUTING.md 표를 따른다.
3. `Shared.props`, `*.vcxproj`, `CG_Final.slnx`, `Common/`, `freeglut/`는 사용자가 팀 합의를 확인해 주기 전에는 고치지 않는다.
4. 소스 파일은 폴더에 넣기만 하면 자동으로 빌드에 포함된다. `.vcxproj`에 `ClCompile`을 추가하지 않는다.
5. 빌드: Visual Studio 2026, **Debug | x86(Win32)**. PR 전에 빌드가 되는지 확인한다.
6. 파일 경로는 상대경로(`"Models/xxx.dat"`). C++ 소스는 UTF-8(BOM).
7. 빌드 결과물(`Bin/`, `Build/`, `.vs/`, exe), 개인정보, 개인 메모(`CLAUDE.local.md`)는 커밋하지 않는다.
8. 사용자가 시연 때 직접 설명할 수 있도록, 작성한 코드의 원리(수식·행렬·OpenGL 호출)를 설명해 준다.
9. **AI 사용 기록을 남긴다** (CONTRIBUTING.md 6번). 작업을 마칠 때마다 저장소 루트의 `AI_LOG.local.md`에 한 줄을 추가하고(커밋하지 않음), PR을 올릴 때 PR의 "AI 사용" 칸과 `docs/ai-log/<담당>.md`의 발표용 요약을 쓴다. 사용자가 다른 기록 위치를 정해 두었으면 그곳에 적는다. **사용자가 실제로 한 일만 적고, 모르면 묻는다.**
10. 규칙이 애매하거나 공용 파일을 바꿔야 하면 **멈추고 사용자에게 묻는다.**
