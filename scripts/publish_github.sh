#!/usr/bin/env bash
# Publish a separate copy to a public repository under the authenticated account.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
for tool in gh git python3; do
  command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 127; }
done
gh auth status >/dev/null 2>&1 || { echo "Run gh auth login first." >&2; exit 2; }
OWNER="$(gh api user --jq '.login')"
REPO_NAME="${BOND_GITHUB_REPO_NAME:-bond-analytics-cpp}"
REPO="${OWNER}/${REPO_NAME}"
REPO_URL="https://github.com/${REPO}"

if gh repo view "$REPO" >/dev/null 2>&1; then
  VISIBILITY="$(gh api "repos/${REPO}" --jq '.visibility')"
  [[ "$VISIBILITY" == "public" ]] || { echo "Refusing to change repository visibility." >&2; exit 3; }
else
  gh repo create "$REPO" --public --description 'C++17 bond analytics with tests, benchmarks, Doxygen and Actions'
fi

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  if git remote get-url origin >/dev/null 2>&1; then
    CURRENT_ORIGIN="$(git remote get-url origin)"
    case "$CURRENT_ORIGIN" in
      "$REPO_URL"|"https://github.com/${REPO}.git"|"git@github.com:${REPO}.git"|"git@github.com:${REPO}") ;;
      *) echo "origin points to a different repository: $CURRENT_ORIGIN" >&2; exit 4 ;;
    esac
  fi
else
  git init -b main
fi

python3 scripts/configure_repository.py --url "$REPO_URL"
[[ "$(git branch --show-current)" == "main" ]] || git branch -M main
git config user.name >/dev/null 2>&1 || git config user.name "$OWNER"
git config user.email >/dev/null 2>&1 || git config user.email "$(gh api user --jq '.id')+${OWNER}@users.noreply.github.com"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "${REPO_URL}.git"
git add -A
if ! git rev-parse --verify HEAD >/dev/null 2>&1; then
  git commit -m 'Add bond analytics project with automated verification'
elif ! git diff --cached --quiet; then
  git commit -m 'Update bond analytics project and publishing setup'
fi
git push -u origin main

# Enabling Pages can require additional permissions or one manual Settings change.
if gh api "repos/${REPO}/pages" >/dev/null 2>&1; then
  gh api --method PUT "repos/${REPO}/pages" -f build_type=workflow >/dev/null || true
else
  gh api --method POST "repos/${REPO}/pages" -f build_type=workflow >/dev/null || true
fi
echo "Source: ${REPO_URL}"
echo "Actions: ${REPO_URL}/actions"
echo "Expected Pages URL: https://$(printf '%s' "$OWNER" | tr '[:upper:]' '[:lower:]').github.io/${REPO_NAME}/"
