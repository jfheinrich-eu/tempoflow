# Pull Request Approval Policy

Changes to `main` require at least one approving review from a code owner. A pull-request author cannot satisfy that requirement with their own review.

## Temporary single-maintainer operation

While `jfheinrich` is the only human maintainer, `jfheinrich-bot` provides a formal approval for eligible pull requests. This approval does not replace engineering review. Copilot review, CI, commit-policy validation, and CodeQL remain separate controls.

The bot approval workflow accepts a pull request only when all of these conditions are true:

- the author is `jfheinrich`;
- the pull request is not a draft;
- the source branch belongs to this repository, not a fork;
- the evaluated commit is still the current pull-request head;
- `windows-build-and-test`, `validate`, and `Analyze C++` completed successfully;
- `jfheinrich-bot` has not already approved the same head commit.

The workflow does not check out or execute pull-request code. It runs from the default branch after the required workflows complete.

## Bot credentials

Create a fine-grained personal access token while signed in as `jfheinrich-bot`. Limit repository access to `jfheinrich-eu/tempoflow` and grant only `Pull requests: Read and write`. Choose a short expiration and rotate the token before it expires.

Store the token as a repository Actions secret. Do not paste it into an issue, pull request, commit, workflow, or chat message.

```powershell
gh secret set BOT_REVIEW_TOKEN --repo jfheinrich-eu/tempoflow
```

The command requests the token interactively without storing it in the shell history.

## Main branch rules

The `main-merge-required` ruleset must enforce:

- one approving review;
- code-owner review;
- dismissal of stale approvals after new commits;
- approval of the most recent push by someone other than its author;
- no permanent administrator bypass.

Do not enable these final restrictions until `BOT_REVIEW_TOKEN` is configured and the approval workflow is present on the default branch. The first pull request that introduces the workflow requires a manual bootstrap merge because `workflow_run` workflows execute only when their definition exists on the default branch.
