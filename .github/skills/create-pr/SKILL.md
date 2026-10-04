---
name: create-pr
description: Create a TempoFlow pull request using the repository template and verified project conventions.
disable-model-invocation: true
---
Create a pull request for the current TempoFlow branch. Do not make unrelated code changes.

## 1. Preflight and scope

1. Resolve the repository root and confirm that the current directory is inside it.
2. Read `.github/copilot-instructions.md`, `CONTRIBUTING.md`, `.github/pull_request_template.md`, and any matching path-specific instructions for files in the branch diff.
3. Confirm that `HEAD` is attached to a feature branch and that no merge, rebase, cherry-pick, revert, or bisect is in progress. Stop and report if a precondition fails; do not create a branch automatically.
4. Inspect `git status --short --untracked-files=all`, `git diff`, and `git diff --cached`. A pull request contains committed changes only. If there are staged, unstaged, or untracked changes, do not stage, commit, stash, discard, or silently omit them. Explain which changes are outside the PR and ask whether to proceed with the committed scope or stop.
5. Determine the intended base branch from explicit user direction, branch configuration, and the repository default. Do not guess a non-default target. Compare the current branch with that base and inspect the commits and complete diff that the PR would contain.
6. Check whether an open pull request already exists for this head branch. If one exists, report its URL and ask whether the user wants to update it; if approved, update its title/body from the verified diff and current template using `gh pr edit` or the authenticated GitHub integration, then verify it instead of creating a duplicate.
7. Review a few recent, relevant, merged TempoFlow pull requests for conventions. Prefer human-authored PRs for comparable changes over automated dependency updates. In particular, use prior workflow/documentation PRs for scope and verification style and recent implementation PRs for risk reporting. Use the authenticated GitHub CLI or an available GitHub integration. If neither can access PR history, say so and do not imply that prior PRs were reviewed. Treat the current template as authoritative; do not copy old body structures that conflict with it.
8. Identify linked issues only from explicit user context, branch/commit references, or confirmed repository issues. Never invent issue numbers or add `Closes`/`Fixes` references speculatively.
9. Check the diff for unrelated changes, generated output, secrets, personal or machine-specific data, and accidental binaries. Do not expose secret values. If suspicious content may be included, stop and report the affected paths.

## 2. Verify the proposed change

1. Select focused checks based on the actual diff. Consult the repository's documented commands and existing test/build setup; do not infer test commands from another project.
2. Run safe, relevant checks when feasible, including `git diff --check`. Report each exact command and its actual result. Never claim a check passed unless it ran successfully.
3. For checks not run, state that clearly and give the reason. For documentation-only or other non-code changes, explain why builds or tests were not applicable instead of implying they passed.
4. If required checks are failing, unresolved, or materially incomplete, do not describe the PR as fully verified. Ask whether to create it as a draft or wait for the checks when that choice is not clear from the user's request.

## 3. Prepare the PR title and body

1. Use an English title following the repository's established Conventional Commit-style PR titles, such as `feat(scope): add concise capability` or `docs(scope): clarify behavior`. Match the main purpose of the branch; do not include a trailing period.
2. Read `.github/pull_request_template.md` directly before preparing the body. The template is mandatory. Preserve every heading, checklist item, and their order. Replace each instruction or placeholder with accurate, concise content; do not omit sections or substitute a generic description.
3. Use the template's exact section order:
   - **Summary:** Explain the problem and chosen solution. Use focused bullets when several meaningful changes need listing, following relevant recent TempoFlow PRs.
   - **Scope:** Keep every template checklist item. Check an item only when the branch diff supports it; otherwise leave it unchecked and explain any non-applicability or remaining work nearby.
   - **Verification:** Keep every template checklist item. Mark only completed checks as checked. Include the exact commands run and their results, and list checks not run with reasons. Distinguish local results from GitHub Actions or other checks that will run after creation.
   - **Risk:** Describe actual timing, thread-safety, compatibility, security, and rollout risks relevant to this change. State meaningful residual risks; do not use unsupported assurances such as “no risk.”
4. Keep the body accurate to the committed diff and actual evidence. Mention documentation, changelog, compatibility, and tests only when verified. Do not copy claims or results from earlier PRs.
5. If the required template is missing or cannot be read, stop and report the problem instead of composing a template-free body.

## 4. Create and verify

1. Prefer the authenticated GitHub CLI for this repository. Use `gh pr create --title ... --body-file ...` with a temporary body file populated from the completed template; keep that file outside the repository and remove only that exact temporary file after the CLI has finished. Do not embed a multiline body in fragile shell quoting.
2. If the GitHub CLI is unavailable, an authenticated GitHub integration or the GitHub VS Code extension may be used when it is available and provides a suitable PR creation flow. Use the same title and template-derived body, and verify the result. Do not assume an extension is installed or authenticated, and do not create a second PR through another client. If no authenticated creation method is available, stop and report that requirement.
3. Before creating, confirm the destination repository, head branch, and base branch. If the branch is not pushed, do not force-push or publish it to an unexpected remote or fork. Confirm the intended remote and obtain user approval before a push if the target is not unambiguously established. Never amend commits, alter staging, or push unrelated branches.
4. After creation, retrieve the PR details and verify the URL, title, head/base branches, and body. Report any mismatch explicitly rather than claiming success. Do not approve or merge the PR.

## Finish

Report the created PR URL, title, base and head branches, checks actually performed and not performed, and any remaining CI/review steps. If creation did not complete, state exactly where it stopped and why. Do not commit changes made during this workflow or claim that the PR is ready to merge based solely on local checks.
