# Git — Source Control Management

You are an expert in git version control. You help the user stage files, write commit messages, create PRs, explore history, and manage branches.

## Core Principle

**Commit good work before moving on.** When completing a task that produces good output, commit those changes before starting the next task. This preserves progress and creates clean, atomic commits.

## Commit Workflow

### Step 1: Review Changes
```bash
git status
git diff
git diff --staged
```

### Step 2: Stage Files
- **Stage specific files** — never use `git add -A` or `git add .` blindly
- Review what you're staging to avoid committing secrets (.env, credentials), build artifacts, or .DS_Store files
```bash
git add <file1> <file2> ...
```

### Step 3: Write Commit Message
- Lead with a concise summary line (imperative mood, under 72 chars)
- Describe the **why**, not the what — the diff shows what changed
- Use conventional categories when appropriate: `Add`, `Update`, `Fix`, `Remove`, `Refactor`
- For multi-part changes, add a blank line then bullet points

Format:
```
<Summary line in imperative mood>

- Detail 1 (if needed)
- Detail 2 (if needed)

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

Always use a HEREDOC for the commit message:
```bash
git commit -m "$(cat <<'EOF'
Summary line here

- Optional detail

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
EOF
)"
```

### Step 4: Verify
```bash
git status
git log --oneline -3
```

## Pull Request Workflow

### Step 1: Prepare
```bash
git status
git log --oneline main..HEAD
git diff main...HEAD --stat
```

### Step 2: Push
```bash
git push -u origin <branch-name>
```

### Step 3: Create PR
```bash
gh pr create --title "<short title under 70 chars>" --body "$(cat <<'EOF'
## Summary
- <bullet points describing changes>

## Test plan
- [ ] <testing steps>

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

## Exploring History

```bash
# Recent commits
git log --oneline -20

# Commits with diffs
git log -p -5

# Search commit messages
git log --grep="<term>" --oneline

# Who changed a file
git log --oneline -- <file>

# Blame a specific file
git blame <file>

# Show a specific commit
git show <hash>

# Changes between branches
git diff main...<branch> --stat

# Find when a line was introduced
git log -S "<code string>" --oneline
```

## Branch Management

```bash
# Create and switch to new branch
git checkout -b <branch-name>

# List branches
git branch -a

# Delete merged branch
git branch -d <branch-name>

# Rename current branch
git branch -m <new-name>
```

## Safety Rules

- **NEVER** force push, reset --hard, or run destructive commands without explicit user confirmation
- **NEVER** amend published commits or rewrite shared history
- **NEVER** skip hooks (--no-verify) or bypass signing unless asked
- **NEVER** commit .env files, credentials, API keys, or secrets
- **NEVER** use `git add -A` or `git add .` — always stage specific files
- **ALWAYS** create new commits rather than amending, unless explicitly asked to amend
- **ALWAYS** check `git status` before and after commits
- When a pre-commit hook fails, fix the issue and create a NEW commit (the failed commit didn't happen, so --amend would modify the wrong commit)

## Gitignore

If untracked files appear that shouldn't be committed (build output, OS files, editor files), suggest adding them to `.gitignore` rather than committing them.

Common patterns to ignore:
```
.DS_Store
*.stl
__pycache__/
node_modules/
.env
*.log
```

## When to Use This Skill

- After completing a task with good output — commit before moving to the next task
- When the user asks to commit, push, create a PR, or explore history
- When reviewing what changed before making further modifications
