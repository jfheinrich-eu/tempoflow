# Commit Policy

TempoFlow accepts only atomic commits that follow Conventional Commits 1.0.0 and include a Developer Certificate of Origin sign-off.

## Required format

Use this structure:

```text
<type>[optional scope][!]: <imperative description>

[optional body]

[optional footers]
Signed-off-by: Your Name <your.email@example.com>
```

Allowed types are `feat`, `fix`, `docs`, `test`, `build`, `ci`, `refactor`, `perf`, `style`, `chore`, and `revert`. Use lowercase types and scopes. Do not end the subject with a period.

Create the sign-off automatically:

```powershell
git commit -s
```

Breaking changes require both the `!` marker and a `BREAKING CHANGE:` footer:

```text
feat(preset)!: replace the preset contract

BREAKING CHANGE: Consumers must migrate saved presets to version 2.

Signed-off-by: Your Name <your.email@example.com>
```

## Atomic grouping

Each commit must represent one coherent purpose. Assign each changed file to exactly one commit group. Do not combine unrelated source, build, policy, or documentation work in a catch-all commit.

Use the repository `/commit` prompt when Copilot should prepare the groups interactively. A commit created directly from an editor or review interface does not run that prompt automatically and remains subject to the same validation rules.

## Enforcement

The repository uses two checks:

- `.githooks/commit-msg` rejects invalid messages before a local commit is created.
- `.github/workflows/commit-policy.yml` validates every commit in a pull request.

Run the development setup script once per clone to activate the version-controlled hooks:

```powershell
.\scripts\setup-dev.ps1
```

The script configures the local repository to use `.githooks`. Verify the setting with:

```powershell
git config --local --get core.hooksPath
```

The expected output is `.githooks`.

The setup script does not overwrite another configured hooks path. Resolve that conflict explicitly before activating the repository hooks.

Do not use `--no-verify`. If a commit is rejected, correct its message or sign-off and retry.

## Manual validation

Validate an existing message file:

```powershell
.\scripts\validate-commit-message.ps1 -MessageFile .git\COMMIT_EDITMSG
```

Run the policy regression tests:

```powershell
.\tests\commit-message-policy.tests.ps1
```
