 
#!/bin/bash
# fix_frontAndBack.sh — устанавливает тип empty для патча frontAndBack
# в constant/polyMesh/boundary и во всех файлах полей 0/ (или 0.orig/)
# Для OpenFOAM v2306
# Usage: ./fix_frontAndBack.sh [CASE_DIR]

set -euo pipefail

CASE_DIR="${1:-.}"

echo "Исправление типа грани frontAndBack → empty"
echo "Case: $CASE_DIR"
echo

python3 - "$CASE_DIR" << 'PYEOF'
import os, re, sys, glob

case_dir = sys.argv[1]
changes = []

# ── 1. constant/polyMesh/boundary ──────────────────────────────
boundary = os.path.join(case_dir, "constant", "polyMesh", "boundary")
if os.path.isfile(boundary):
    with open(boundary, "r") as f:
        content = f.read()

    # Меняем только type внутри блока frontAndBack { ... },
    # сохраняя nFaces, startFace и прочие ключи
    def fix_boundary_block(m):
        block = m.group(0)
        return re.sub(
            r'type\s+(symmetry|symmetryPlane)\s*;',
            'type            empty;',
            block
        )

    new_content = re.sub(r'frontAndBack\s*\{[^}]*\}', fix_boundary_block, content)

    if new_content != content:
        with open(boundary, "w") as f:
            f.write(new_content)
        changes.append(f"  ✓ {boundary}")
    else:
        print(f"  — {boundary} (без изменений)")
else:
    print(f"  ! {boundary} не найден")

# ── 2. Файлы полей в 0/ и/или 0.orig/ ───────────────────────────
zero_dirs = []
for d in ("0", "0.orig"):
    p = os.path.join(case_dir, d)
    if os.path.isdir(p):
        zero_dirs.append(p)

for zero_dir in zero_dirs:
    for field_file in sorted(glob.glob(os.path.join(zero_dir, "*"))):
        if not os.path.isfile(field_file):
            continue
        with open(field_file, "r") as f:
            content = f.read()

        # Полностью заменяем содержимое блока frontAndBack на type empty
        new_content = re.sub(
            r'(frontAndBack\s*\{)([^}]*)(\})',
            r'\1\n        type            empty;\n    \3',
            content
        )

        if new_content != content:
            with open(field_file, "w") as f:
                f.write(new_content)
            changes.append(f"  ✓ {field_file}")
        else:
            print(f"  — {field_file} (без изменений)")

print()
if changes:
    print(f"Внесено изменений: {len(changes)}")
    for c in changes:
        print(c)
else:
    print("Изменений не требуется.")
PYEOF

echo
echo "Готово. Теперь запустите: decomposePar -force"
