Prepare and create reviewed atomic commits for the current repository. Follow all repository instructions. Do not modify project files while running this command.

## Safety rules

- Run only inside the current Git repository and resolve its root before inspecting files.
- Stop if a merge, rebase, cherry-pick, revert, or bisect is in progress, or if `HEAD` is detached. Explain the state without changing it.
- Never discard or overwrite working-tree changes. Never stash, amend, rebase, reset, clean, push, or force-push.
- Change existing staging only after the explicit authorization described below. When authorized, use `git restore --staged -- <exact-file-paths>` and never alter the working tree.
- Never use `git add .`, `git add -A` without exact file pathspecs, or another command that can stage files outside the approved group.
- Never bypass hooks or checks with `--no-verify`.
- Do not expose secret values. If credentials, private keys, tokens, personal data, generated build output, or suspicious binary files may be included, stop before staging and report only the affected paths and risk.
- Do not commit until the user has approved the exact group, file list, and commit message.
- If the current interface cannot pause for interactive decisions, stop without changing the index or creating commits.

## 1. Inspect repository state

1. Run `git status --porcelain=v1 -z --untracked-files=all`, `git diff`, and `git diff --cached`.
2. Use null-delimited Git output or equivalent argument-safe handling so spaces and special characters in paths remain intact.
3. Detect staged, unstaged, partially staged, renamed, deleted, and untracked files.
4. Read the relevant contents and diffs before proposing groups.
5. Run safe secret, generated-file, oversized-file, and repository-policy checks before staging anything.

If changes are already staged, show their exact paths and ask the user to choose one of these actions:

- preserve the existing index as one indivisible first commit group; or
- explicitly authorize non-destructive unstaging so the files can be regrouped.

Do not alter existing staging without that authorization. If regrouping is approved, unstage only the exact approved paths with `git restore --staged -- <exact-file-paths>`. If a file is partially staged, explain that the one-file-one-group rule requires the complete file to belong to one group and ask how to proceed.

## 2. Select untracked files interactively

1. Obtain every untracked, non-ignored file with `git ls-files --others --exclude-standard -z`.
2. Present a numbered list with repository-relative path, file type, and size. Do not print file contents that may contain secrets.
3. Ask the user to select individual numbers, `all`, or `none`.
4. Confirm the resolved selection once. Exclude every unselected untracked file from all groups and staging commands.
5. If no untracked files exist, say so and continue without asking an unnecessary question.

## 3. Build atomic commit groups

The candidate set consists of tracked changes plus only the selected untracked files.

1. Group files by one coherent purpose, such as product behavior, tests, documentation, build configuration, CI, or repository policy.
2. Account for dependencies and order groups so every intermediate commit is understandable and buildable where practical.
3. Assign every candidate file to exactly one group. A path must never appear in two groups.
4. Do not use hunk-level splitting to work around the one-file-one-group rule.
5. If a single file contains unrelated concerns and cannot form an atomic file-level commit, stop and ask the user to split it manually or explicitly assign the complete file to one group.
6. Keep tests with the behavior they verify when the one-file rule permits it.
7. Do not create an empty or catch-all commit.

For every group, prepare:

- a short group name and rationale;
- the exact repository-relative file list;
- a concise diff summary and risk assessment;
- relevant checks to run;
- one English Conventional Commit message.

Use this message structure:

```text
<type>[optional scope][!]: <imperative description>

[optional body explaining why, behavior, risk, or verification]

[optional footers]
```

Use `feat` for user-visible capabilities and `fix` for defects. Other allowed lowercase types include `docs`, `test`, `build`, `ci`, `refactor`, `perf`, `style`, `chore`, and `revert`. Keep the subject concise and omit a trailing period.

For an incompatible change, use both forms so GitHub and SemVer tooling can recognize it:

```text
<type>[optional scope]!: <description>

BREAKING CHANGE: <precise migration impact and required action>
```

## 4. Obtain approval and commit

Process one group at a time. Show the group name, rationale, exact files, complete proposed commit message, risks, and intended checks. Ask the user to approve or reject it.

- On approval, stage only the exact approved file paths with argument-safe pathspec handling.
- On rejection, ask whether to revise the commit message or skip the complete group. Do not stage or commit a skipped group.
- If the user changes the file list, recompute group uniqueness and show the revised group again.
- Before committing, compare the staged paths with the approved list and show `git diff --cached --stat` plus the staged diff summary.
- Run `git diff --cached --check` and the relevant focused tests. Run broader tests when risk or project instructions require them.
- If a check or hook fails, do not commit by default. Keep the current state, report the failure, and ask the user how to proceed.
- Create the approved commit with `git commit -s`. Pass the message without unsafe shell interpolation.
- After each commit, record its hash and subject. Verify that its changed paths exactly match the approved group before continuing.

Refresh repository status after every commit because hooks may change files. Newly created or modified files require a new interactive review and must not be silently added to a previously approved group.

## 5. Finish

Report created commit hashes and subjects, skipped groups, remaining staged or unstaged changes, excluded untracked files, and checks performed. Do not push. If nothing was approved or no committable changes exist, create no commit and state that clearly.
