#!/usr/bin/env bash
#
# lint-yaml.sh - Validate all YAML files in the repository
#
# Usage:
#   ./scripts/lint-yaml.sh           # Validate all YAML files
#   ./scripts/lint-yaml.sh analysis/ # Validate YAML in specific directory
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

TARGET_DIR="${1:-.}"

echo "🔍 Validating YAML files in: $TARGET_DIR"
echo ""

# Find all YAML files
YAML_FILES=$(find "$TARGET_DIR" -type f \( -name "*.yml" -o -name "*.yaml" \) | sort)

if [ -z "$YAML_FILES" ]; then
    echo -e "${YELLOW}No YAML files found${NC}"
    exit 0
fi

FILE_COUNT=$(echo "$YAML_FILES" | wc -l | tr -d ' ')
echo "Found $FILE_COUNT YAML file(s)"
echo ""

# Validate using Python
python3 << 'PYTHON_SCRIPT'
import yaml
import sys
from pathlib import Path

target = sys.argv[1] if len(sys.argv) > 1 else "."
errors = []
valid = 0

for yml_file in Path(target).rglob("*.yml"):
    try:
        with open(yml_file) as f:
            yaml.safe_load(f)
        print(f"  \033[0;32m✓\033[0m {yml_file}")
        valid += 1
    except yaml.YAMLError as e:
        print(f"  \033[0;31m✗\033[0m {yml_file}")
        errors.append((yml_file, e))

for yml_file in Path(target).rglob("*.yaml"):
    try:
        with open(yml_file) as f:
            yaml.safe_load(f)
        print(f"  \033[0;32m✓\033[0m {yml_file}")
        valid += 1
    except yaml.YAMLError as e:
        print(f"  \033[0;31m✗\033[0m {yml_file}")
        errors.append((yml_file, e))

print("")
if errors:
    print(f"\033[0;31m✗ {len(errors)} file(s) with errors:\033[0m")
    print("")
    for f, e in errors:
        print(f"  {f}:")
        for line in str(e).split('\n'):
            print(f"    {line}")
        print("")
    sys.exit(1)
else:
    print(f"\033[0;32m✓ All {valid} YAML file(s) valid\033[0m")
    sys.exit(0)
PYTHON_SCRIPT
