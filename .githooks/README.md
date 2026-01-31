# Git Hooks

## Installation

```bash
# Set the hooks directory
git config core.hooksPath .githooks

# Or manually copy to .git/hooks/
cp .githooks/* .git/hooks/
chmod +x .git/hooks/*
```

## Hooks

### pre-commit
- Validates all staged YAML files
- Blocks commit if YAML is invalid

### pre-push
- Warns when pushing directly to main
- Checks feature branches for unexpected commit counts
- Warns if branch is far behind main

## Clean PR Workflow

**The Problem:** Feature branches can accidentally include unrelated main commits if created from a stale main or force-pushed incorrectly.

**The Solution:**

```bash
# 1. Start from fresh main
git checkout main
git pull origin main

# 2. Create feature branch
git checkout -b fix/my-fix

# 3. Make changes and commit
git add -A
git commit -m "Fix the thing"

# 4. Push feature branch
git push -u origin fix/my-fix

# 5. Create PR on GitHub (use squash merge)
gh pr create --title "Fix the thing" --body "..."

# 6. After merge, clean up
git checkout main
git pull origin main
git branch -d fix/my-fix
```

## If You Mess Up

If your PR contains unexpected commits:

```bash
# Check what's in your branch vs main
git log --oneline origin/main..HEAD

# If it has junk, create a clean branch
git checkout main && git pull
git checkout -b fix/my-fix-clean

# Cherry-pick only YOUR commits
git cherry-pick abc123 def456

# Push clean branch, create new PR
git push -u origin fix/my-fix-clean
```

## Guardrails

The pre-push hook will warn if:
- **>5 commits ahead**: Might include main commits
- **>10 commits behind**: Should rebase first
- **Direct push to main**: Allowed but noted
