#!/bin/bash
set -e

REMOTE_URL="$1"

if [ -z "$REMOTE_URL" ]; then
  echo "Использование: ./push_repo.sh git@github.com:kol1978/Backward-Facing.git"
  exit 1
fi

BRANCH=$(git rev-parse --abbrev-ref HEAD)

echo "Текущая ветка: $BRANCH"
echo "URL remote: $REMOTE_URL"

# 1. Привязать remote
git remote remove origin 2>/dev/null || true
git remote add origin "$REMOTE_URL"

# 2. Список коммитов от старого к новому
COMMITS=$(git rev-list --reverse HEAD)
TOTAL=$(echo "$COMMITS" | wc -l)
COUNT=0

# 3. Пушить по одному коммиту
for COMMIT in $COMMITS; do
  COUNT=$((COUNT + 1))
  echo ""
  echo "============================================"
  echo "Пуш $COUNT из $TOTAL"
  echo "Коммит: $COMMIT"
  echo "============================================"
  git push --force origin "$COMMIT:$BRANCH"
done

echo ""
echo "Готово! Все $TOTAL коммитов запушены в $BRANCH"
