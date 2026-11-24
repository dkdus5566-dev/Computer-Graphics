#!/usr/bin/env bash
# PR이 팀 규칙을 지켰는지 확인한다 (GitHub Actions에서 실행).
#   실패(error) : 모델을 올리면서 발표용 AI 기록이 없거나, 개인 메모가 들어간 경우
#   경고(warning): main 최신 아님, 커밋 메일 공개, 공용 파일 변경
# 사용법: pr_rules.sh <base 커밋> <PR 커밋>
set -u
base="$1"
head="$2"
fail=0

error()   { echo "::error::$1";   echo "- ❌ $1" >> "${GITHUB_STEP_SUMMARY:-/dev/null}"; fail=1; }
warning() { echo "::warning::$1"; echo "- ⚠️ $1" >> "${GITHUB_STEP_SUMMARY:-/dev/null}"; }

mb=$(git merge-base "$base" "$head") || { echo "::error::merge-base를 찾지 못함"; exit 1; }
files=$(git -c core.quotepath=off diff --name-only --diff-filter=ACMR "$mb" "$head")

if [ "$mb" != "$(git rev-parse "$base")" ]; then
  warning "main이 최신이 아닙니다. 사용자에게 확인한 뒤 git fetch, git merge origin/main 하고 AGENTS.md와 CONTRIBUTING.md를 다시 읽으세요."
fi

# 모델 파일을 올리면 담당의 발표용 AI 기록이 있어야 한다
while read -r prefix log; do
  if grep -qE "^Models/${prefix}_" <<< "$files" && ! git cat-file -e "$head:docs/ai-log/$log.md" 2>/dev/null; then
    error "Models/${prefix}_* 를 올렸지만 docs/ai-log/$log.md 가 없습니다. CONTRIBUTING.md 6번을 읽고 docs/ai-log/_TEMPLATE.md를 복사해 작성하세요. 사용자가 실제로 한 일만 적고, 모르면 사용자에게 물으세요."
  fi
done <<'EOF'
lamp lamp
hermes hermes-shoes
argos argos-eye
ariadne ariadne-thread
EOF

if grep -qE '\.local\.md$' <<< "$files"; then
  error "개인 메모(*.local.md)가 커밋에 들어 있습니다. 공개 저장소이므로 빼야 합니다."
fi

# 메일 주소 자체는 출력하지 않는다
if git log --no-merges --format='%ae%n%ce' "$mb..$head" | grep -vqE '@users\.noreply\.github\.com$|^noreply@github\.com$'; then
  warning "커밋에 공개되면 안 되는 메일 주소가 있을 수 있습니다. 이 저장소의 git user.email을 GitHub noreply 주소로 바꾸고, 이미 올린 커밋은 사용자에게 알리세요."
fi

if grep -qE '^(Common/|SORModeler/|freeglut/|Shared\.props$|CG_Final\.slnx$|.*\.vcxproj$)' <<< "$files"; then
  warning "공용 파일이 바뀌었습니다. 팀 합의를 확인했는지 PR 설명에 적으세요."
fi

[ "$fail" -eq 0 ] && echo "규칙 확인 통과 (경고가 있으면 위 내용을 확인)"
exit "$fail"
