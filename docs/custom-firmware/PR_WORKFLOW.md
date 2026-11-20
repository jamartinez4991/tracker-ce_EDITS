# Feature PR workflow

Procedure for every custom-firmware feature PR in this fork. Status of each feature is tracked in
[`INTEGRATION_LEDGER.md`](INTEGRATION_LEDGER.md); build rules are in [`BUILD_VALIDATION.md`](BUILD_VALIDATION.md).
This fork is separate from the upstream-contribution project: never open PRs against `polyend/tracker-ce`.

**Merging always needs the owner's explicit authorization, separately from technical approval.** A successful build, a
clean rebase, an approved report or "ready to merge" is never permission to merge. Only the stage that is authorized
may be performed; the rest wait.

## Stage A — Inspection (no approval needed)

Read-only. Do not rebase, edit or push.

1. Record `main` SHA, the PR head, base branch and base SHA (`gh api repos/<fork>/pulls/<n>`).
2. Read the full diff against current `main`. Confirm it still contains the reviewed implementation.
3. List shared source files and functions (ledger "shared code" table) and the feature flags.
4. Check dependencies and the integration order in the ledger.
5. Dry-run the merge: `tools/cfw/conflicts.sh` or `git merge-tree --write-tree --name-only origin/main <head>`.
6. Identify interactions with merged features and with other open PRs.
7. **Compare against the integrated `custom-firmware` branch** for the affected functions. A clean merge is not proof
   that features work together: integration glue (for example a reset call that belongs in another feature's
   function) is invisible to Git. Look for glue that exists in `custom-firmware` but in neither PR.
8. Report findings and the proposed resolution. Stop.

## Stage B — Integration preparation (after explicit authorization)

1. Verify the starting state matches what was authorized (head, base, `main`). Stop and report if it differs.
2. Keep a recoverable reference to the original head (`git branch backup/<feature>-pre-rebase <head>`).
3. Rebase onto the **current `main`**, never onto another PR's branch (stacked PRs are rebased after their parent merges).
4. Resolve conflicts preserving both implementations; do not drop or simplify either side. Apply any agreed
   integration glue.
5. Compare the affected functions with `custom-firmware`; look for duplicated or missing integration code.
6. Inspect the whole diff: no conflict markers, no unrelated files, same added lines as the reviewed version apart from
   the resolutions.

## Stage C — Validation

1. Select configurations with `tools/cfw/matrix.py`, plus any extra pair the shared-code table calls for.
2. Reuse results only under the rules in `BUILD_VALIDATION.md` (`tools/cfw/reuse.sh`); build the rest.
3. Run `tools/cfw/hosttests.sh`.
4. Check flag-on and flag-off: the stock build must have no warnings beyond the upstream baseline.
5. Update the ledger and `build-results.tsv`, keeping compiled / host-test / review / hardware separate.

## Stage D — Technical review report

Concise, and explicit about what was *not* tested:

* feature purpose; PR, branch, old and new head SHA; base SHA
* files changed (+/−)
* dependencies and integration order
* conflict resolutions and added glue, with rationale
* shared-code interactions and the comparison with `custom-firmware`
* build table (device × config) with errors and warnings; new warnings
* host tests run and results
* hardware tests still required
* known risks and limitations
* GitHub mergeability (treat as a hint only)

## Stage E — Merge (explicit authorization only)

1. Re-verify head, base, `main`, mergeability and the file list immediately before merging; stop on any difference.
2. Use a merge commit (the method used for all earlier PRs).
3. After merging verify the merge commit and new `main` SHA, that the merged tree matches the reviewed head, and that
   no other PR or branch moved. Report the mergeability of the remaining open PRs.
4. Update the ledger. Do not start the next PR until told to.

## Rules

* Never merge without explicit authorization; never modify or rebase an unrelated PR or branch.
* Never force-push without authorization for that operation; use `--force-with-lease` pinned to the old head.
* Never resolve a conflict by discarding an existing feature implementation.
* Never claim hardware validation from builds or host tests.
* Stacked PRs: merging a stacked PR merges only into its base *branch*. If the parent is merged to `main` first, the
  child's code never reaches `main` (this happened with #2, #4, #5 and #6; see the ledger). Retarget or reopen the child
  against `main` before merging the parent.
* Keep infrastructure changes out of feature PRs and feature changes out of infrastructure PRs.

## Feature registration and CI

`tools/cfw/features.tsv` is the authoritative registry; nothing is guessed from branch names or flags.

| Situation | PR validation result |
|---|---|
| PR branch registered in `features.tsv`, not yet in `main` | stock, merged, `<feature>` (with its dependencies) and `merged+<feature>`, each on Plus, Original and Mini (12 builds) |
| PR branch **not** registered and not an infrastructure branch | **fails** at the `plan` job with a clear error; no build job runs, so it cannot go green without its feature being compiled |
| Infrastructure PR: branch matches a pattern in `tools/cfw/infra-branches.txt` (currently `infra/*`) | stock and merged only (6 builds); no feature flags to test |
| Registered branch whose feature is already `in_main=y` | fails (stale registry or wrong branch) |
| Manual dispatch with a registered `feature` input | same as a registered PR |
| Manual dispatch with the input empty | stock and merged only |
| Manual dispatch with an unregistered feature name | fails |

So a new feature PR must first add its row to `features.tsv` (a reviewed change); infrastructure-only work uses an `infra/…`
branch name, and a new kind of infrastructure branch needs a reviewed addition to `infra-branches.txt`.
Helper scripts are covered by `tools/cfw/tests/run.sh`, which runs in CI (`selftest` job) and locally.

## Automation

The two workflows added for this process, `.github/workflows/cfw-pr-validate.yml` and `cfw-integration.yml`, are
read-only: `contents: read`, no writes, no comments, they never merge, rebase, push or close anything. The repository
also has the inherited upstream `.github/workflows/build.yml` and others, which have their own permissions and behaviour
(including write access and PR comments); they are not covered by this statement and are not changed by the CFW
infrastructure. `cfw-integration.yml` prints `conflicts.sh` informationally; `conflicts.sh --strict` (non-zero exit on any
missing, unmergeable or conflicting open branch) is available for local or future gating. CI is evidence for Stage C, not
a replacement for Stages A, B and D.
