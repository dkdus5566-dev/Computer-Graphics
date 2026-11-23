# GitHub Copilot 안내

이 저장소에서 작업할 때는 저장소 루트의 `AGENTS.md`와 `CONTRIBUTING.md` 규칙을 따른다.

- main에 직접 commit/push 하지 않고, `feature/<내용>` 브랜치 → PR로 올린다.
- 사용자가 맡은 담당 파일만 고친다. `Shared.props`, `*.vcxproj`, `CG_Final.slnx`, `Common/`, `freeglut/`는 팀 합의 없이 고치지 않는다.
- C++17, OpenGL 고정 파이프라인 + freeglut, Visual Studio 2026 Debug | x86.
- 파일 경로는 상대경로(`"Models/xxx.dat"`), C++ 소스는 UTF-8(BOM).
