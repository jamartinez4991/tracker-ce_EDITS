# Prompt template for feature PR work

Copy the block below, fill the placeholders, and send it. It deliberately refers to the repo documents instead of
repeating them.

```
Work on PR #<PR_NUMBER> — <FEATURE_NAME> in my personal fork jamartinez4991/tracker-ce_EDITS.

Feature branch: <FEATURE_BRANCH>
Declared base: <BASE_BRANCH>            (main for standalone features, the parent PR's branch if stacked)
Current main SHA: <MAIN_SHA>
Expected PR head SHA: <HEAD_SHA>
Task: <REQUESTED_TASK>   (inspect only | rebase and validate | review report | other)
Special integration concerns: <CONCERNS or "none">

Follow docs/custom-firmware/PR_WORKFLOW.md and use docs/custom-firmware/INTEGRATION_LEDGER.md and
docs/custom-firmware/BUILD_VALIDATION.md as the sources of truth.

1. Inspect first. Verify the actual state of main, the PR head/base and mergeability before relying on this prompt;
   stop and report if it differs from the values above. Do not modify anything during inspection.
2. Identify dependencies, shared files/functions and likely conflicts. If the feature is stacked on another PR, say
   whether the parent has actually reached main (a stacked PR merged into its parent's branch has not).
3. Respect the feature's accepted design decisions (ledger "Decisions"). Do not simplify or drop either side to
   resolve a conflict.
4. Compare the affected functions with the integrated `custom-firmware` branch and look for integration glue that is
   missing from the PR. A clean Git merge is not evidence that the combined features work.
5. Rebase only if this prompt's task authorizes it, onto the current main (never onto another PR branch). Keep a
   backup reference to the old head. Push only this feature branch, with --force-with-lease pinned to the old head,
   and only if the task authorizes pushing.
6. Validate with the cumulative matrix (tools/cfw/matrix.py). Reuse earlier results only when tools/cfw/reuse.sh
   says the source tree, flags and toolchain are identical. Run tools/cfw/hosttests.sh. Do not repeat builds
   unnecessarily and do not build every flag combination.
7. Update the ledger and build-results.tsv, keeping compiled / host-test / source-review / hardware separate. Never
   claim hardware validation without hardware testing.
8. Finish with the Stage D technical report from PR_WORKFLOW.md, listing everything not tested.
9. Stop before merging. Merging, and starting the next PR, need my separate explicit authorization.
Do not touch any other PR or branch.
```

## Notes for the person filling it in

* Standalone feature: `BASE_BRANCH = main`. Stacked feature: base is the parent's branch; ask for the parent to be
  merged first and the child rebased onto `main` afterwards.
* For an inspection-only request set the task to "inspect only"; step 5 then does not apply.
* To authorize a merge, say so explicitly in a later message — approving a report is not enough.
