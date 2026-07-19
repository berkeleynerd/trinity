# Upstream sync record

Authoritative record of how far this fork has taken `carbonengine/trinity`, and
what it has deliberately declined. **This file is the sync point** — not
`git merge-base`.

## Why this file exists

`main` is squash-merged: every PR lands as exactly one commit whose only parent
is the previous `main`. That is a deliberate choice (linear history, PR-level
revert, and a queue worker that verifies the reviewed head lands atomically),
and it is not going to change.

The consequence for upstream syncs is that a squash copies upstream's *tree*
but not its *ancestry*. After a sync PR lands, every upstream line is present
in our tree while `git merge-base main upstream/main` still points at whatever
it did before. Anything that derives "what have we taken?" from the commit
graph will therefore be wrong.

So the sync point is recorded here as data instead. Read this file; do not ask
git.

## Current state

| | |
|---|---|
| Last upstream commit taken | `8d68bc93a` — *Merge pull request #25 from carbonengine/PLAT-11614-Mesh-mirroring-of-child-meshes* |
| Landed by | PR #27 (childmesh mirroring sync) |
| Upstream tip at that time | `4f70e3e00` |

## Deliberately not taken

| Declined | Exclude with | What |
|---|---|---|
| upstream PR #27 | `^4f70e3e00` | blue v6.0.0 / core v3.0.0 upgrade |

Covering `64be36108` (the upgrade itself), `08e47c4de` (`Tr2RenderContext`
deprecation warning — `CcpSemaphore` constructed with a name), and `c5d262049`
(compile definition for python2 support, flagging the latest core tracy feature
off).

**The `Exclude with` column is load-bearing** — every entry must be passed as a
`^<sha>` to step 1 of the sync procedure below, or that sync will replay the
declined work. Add a row whenever something is declined, and never remove one: a
decline stays in force until the change is deliberately taken, at which point it
moves to the sync point instead.

We keep pinned blue 5.1.2 / core 2.4.0 at the fixed vcpkg baseline `a831348`
rather than ratchet a major dependency forward as a side effect of a sync.
Carbon packages resolve through the git-registry baseline in
`vcpkg-configuration.json`, **not** the `vcpkg-registry` submodule pin — so a
dependency bump is an explicit edit, and it should stay that way.

The upgrade will land as its own deliberate PR, gated on its own evidence, when
we choose to take it.

## Taking the next sync

Do **not** use `git merge upstream/main`. Its merge-base predates work already
in our tree, so it re-derives changes we have taken and reasons from a base
that does not describe reality.

And do **not** cherry-pick a bare range either. A range is opt-*out*: it takes
everything after the sync point and relies on whoever runs it to remember the
exclusions. This fork permanently declines upstream work, so a range reaching
past a declined commit replays it — the three blue/core commits sit after the
current sync point, so any range ending at `upstream/main` picks them up.
Nothing warns you.

Sync is **opt-in**: list candidates, then take named commits.

```sh
git fetch upstream

# 1. Candidates — new since the sync point, with declined work excluded.
#    Add one ^<sha> per row of the declined table above.
git log --oneline --no-merges <last-taken>..upstream/main ^4f70e3e00

# 2. Take them by name. Never a range.
git checkout -b rebecca/sync-upstream-<topic> origin/main
git cherry-pick <sha> [<sha> ...]
```

Three mechanics worth knowing, all verified against this repo on git 2.55.0:

- `--no-merges` and `^<sha>` belong on the **listing** in step 1. `git log`
  documents both. With the current table, step 1 returns nothing — everything
  upstream has done since `8d68bc93a` is declined work.
- **Do not put `--no-merges` on `git cherry-pick`.** It is not in
  `git cherry-pick -h`; cherry-pick accepts it only because it forwards
  revision arguments to `rev-list`, and that passthrough is incidental rather
  than contractual. It happens to filter merges on 2.55.0 — which is exactly
  what makes it dangerous to write down, since a git that stops accepting it
  changes behaviour silently. Step 2 names commits, so the question does not
  arise.
- Upstream lands its PRs as merge commits, and cherry-pick refuses a merge
  without `-m`. That is a reason to select non-merge commits when listing, not
  to replay merges — a merge carries no content its parents do not. The
  recorded sync SHA may itself be a merge (`8d68bc93a` is); that is fine, it is
  a range endpoint for the listing, never something replayed.

Stopping short of upstream's tip is normal — take the commits you actually want
and record the last of them as the new sync point.

### Guard after every sync

Declines are only real if something checks them. Before opening the PR:

```sh
git diff origin/main -- vcpkg.json vcpkg-configuration.json
```

Expect **no output**. `carbon-core >= 2.4.0`, `carbon-blue >= 5.1.2`, and the
registry baseline `a831348` must be untouched unless the PR's stated purpose is
to move them. Any diff here means declined work came along.

### Declines compound

Each declined change raises the cost of the next sync: upstream commits built on
top of it will not apply cleanly against our pinned tree. A candidate that
conflicts on blue/core APIs is not something to force through — it is the signal
that the decline has reached its expiry and the upgrade should be scheduled as
its own gated PR.

**Update the table above in the same PR as the sync.** A sync that lands
without bumping this file leaves the record lying, and the next sync starts
from a false premise.

## Gating a sync

A sync PR changes the engine, which the probe's own legs cannot see: `run.sh`,
`run_rc07.sh`, and `run_w1d.sh` all compare the Swift host against the ObjC++
host on the *same* build. That is the right gate when the engine is fixed and
the port is the variable — it is structurally blind when the engine is the
variable, because an engine-side change moves both hosts together and every leg
stays green.

Gate a sync with a **cross-merge capture** instead: run the ObjC++ reference
host alone against the pre-merge and post-merge builds and compare across the
boundary.

Sample before concluding. `talocan-hdr` at `hdr-finish` is multi-state
nondeterministic — a single capture from each side is not interpretable, and
comparing one against one will manufacture a difference that is not there.
Take several runs per side and compare the modal state; treat a lone outlier as
a draw from the distribution, not a finding.
