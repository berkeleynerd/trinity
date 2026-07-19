#!/usr/bin/env python3
"""Keep the W1-D journey exports in sync with RenderFrame.

W1-D (trinity #14) gave the Swift host two granular exports that let it
sequence the New Eden tour frame itself:

    TrinityStandaloneProbeAdvanceJourneyFrame   -- RenderFrame's pre-draw
                                                   journey half
    TrinityStandaloneProbeRecordBallparkTelemetry -- RenderFrame's post-draw
                                                   telemetry tail

Both bodies are *verbatim copies* of the corresponding RenderFrame regions.
That duplication is deliberate: RenderFrame stays the internal-path oracle,
and run_w1d.sh's granular-vs-monolith A/B gate is only meaningful if the two
sides are independent code. But it means any edit to RenderFrame's journey
halves silently desynchronises the granular path.

That is not hypothetical. PR #22 (W1-D) was merged out of date with a main
that had taken PR #20, which edited *both* copied regions -- adding the
combatRehearsal weapon-presentation call to the journey half and rewriting
the directional-shadow contract in the tail. The merge was textually clean
and run_w1d.sh stayed byte-identical green (the tour fixtures never exercise
the combat or native-ship shadow paths), so nothing caught it.

This script closes that hole:

    --check     verify each export body still matches its RenderFrame region.
                Exits non-zero on drift. Wire this into CI.
    --generate  (re)build both exports from the current RenderFrame and add
                the header declarations. Use after intentionally editing
                RenderFrame's journey halves.

Regions are located by anchor, never by line number, so the tool survives
unrelated edits elsewhere in the file.
"""

import argparse
import difflib
import re
import sys
from pathlib import Path

CPP = Path("trinity/TrinityStandaloneProbe.cpp")
HDR = Path("trinity/TrinityStandaloneProbeApi.h")

BEGIN = "\t// >>> BEGIN VERBATIM COPY of RenderFrame {name} -- do not hand-edit;"
BEGIN2 = "\t// >>> regenerate with tools/w1d_sync_exports.py --generate <<<"
END = "\t// >>> END VERBATIM COPY of RenderFrame {name} <<<"

JOURNEY = "journey half"
TAIL = "telemetry tail"


def _find(lines, pattern, start=0):
    rx = re.compile(pattern)
    for i in range(start, len(lines)):
        if rx.search(lines[i]):
            return i
    raise SystemExit(f"anchor not found: {pattern!r} (searched from line {start + 1})")


def extract_regions(lines):
    """Return the two RenderFrame regions, located purely by anchor."""
    rf = _find(lines, r"^TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeRenderFrame\(")
    fz = _find(lines, r"^\tif\( !freezeScene \)$", rf)
    anim = _find(lines, r"probe->device->SetAnimationTime", fz)
    last = _find(lines, r"^\tprobe->lastSimTime = simTime;$", anim)
    tail = _find(lines, r"^\tif\( rendered && !freezeScene \)$", anim)
    ret = _find(lines, r"^\treturn rendered &&", tail)
    # journey half: the if(!freezeScene) block through the SetAnimationTime pokes
    # telemetry tail: the SetTemporalHistoryFrozen(false) line through the final return
    return {
        JOURNEY: lines[fz : last + 1],
        TAIL: lines[tail - 1 : ret + 1],
    }


def extract_copies(lines):
    """Return the copied bodies currently embedded in the two exports."""
    out = {}
    for name in (JOURNEY, TAIL):
        begin = BEGIN.format(name=name)
        end = END.format(name=name)
        try:
            b = next(i for i, l in enumerate(lines) if l == begin)
            e = next(i for i, l in enumerate(lines) if l == end)
        except StopIteration:
            return None  # exports absent or unmarked
        out[name] = lines[b + 2 : e]  # skip BEGIN + BEGIN2
    return out


CONTRACT = """
// W1-D: the journey/ballpark/gate crossing, granular. Two additive exports
// that let the Swift host sequence the TOUR frame itself (ballparkMode != OFF)
// instead of calling the monolithic RenderFrame. Per tour frame, in strict
// order exactly once:
//   AdvanceJourneyFrame -> [W1-C driver rungs, captureProducts 0] ->
//   RecordBallparkTelemetry.
//
// Both bodies are VERBATIM COPIES of RenderFrame's two journey halves,
// delimited by BEGIN/END VERBATIM COPY markers. The duplication is
// deliberate -- RenderFrame remains the internal-path oracle so that
// run_w1d.sh's granular-vs-monolith A/B gate compares independent code.
// Do not hand-edit the marked regions: after changing RenderFrame's journey
// halves, run `tools/w1d_sync_exports.py --generate`, and let
// `--check` guard the invariant in CI.
//
// renderedFrameCount stays engine-owned: RecordBallparkTelemetry owns the
// sole per-frame ++, so the host's frame index only computes realTime/simTime.
// Tour-lane invariants: freezeScene == false and
// solarParticlePrewarmCompleted == false.
""".rstrip("\n").split("\n")


def build_exports(regions):
    lines = list(CONTRACT)
    lines += [
        "TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeAdvanceJourneyFrame(",
        "\tvoid* opaqueProbe, int64_t realTime, int64_t simTime )",
        "{",
        "\tauto* probe = static_cast<StandaloneProbe*>( opaqueProbe );",
        "\tif( !probe || !probe->renderContext || !probe->driver )",
        "\t{",
        "\t\treturn false;",
        "\t}",
        "\t// Tour-lane invariant: the journey never freezes, so the copied",
        "\t// if(!freezeScene) guards are always taken.",
        "\tbool freezeScene = false;",
        "\t(void)realTime;",
        BEGIN.format(name=JOURNEY),
        BEGIN2,
    ]
    lines += regions[JOURNEY]
    lines += [
        END.format(name=JOURNEY),
        "\treturn true;",
        "}",
        "",
        "TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeRecordBallparkTelemetry(",
        "\tvoid* opaqueProbe, int64_t realTime, int64_t simTime, bool rendered )",
        "{",
        "\tauto* probe = static_cast<StandaloneProbe*>( opaqueProbe );",
        "\tif( !probe || !probe->renderContext || !probe->driver )",
        "\t{",
        "\t\treturn false;",
        "\t}",
        "\t// `rendered` is the caller's driver-frame result, mirroring RenderFrame's",
        "\t// `if( rendered && !freezeScene )` gate: on a failed draw the copied tail",
        "\t// is skipped and renderedFrameCount does NOT advance, exactly as the",
        "\t// monolith abandons the frame. Do not hard-code this true -- the granular",
        "\t// path would otherwise record telemetry for a frame that never rendered.",
        "\t// The tour never freezes.",
        "\tbool freezeScene = false;",
        "\t(void)realTime;",
        BEGIN.format(name=TAIL),
        BEGIN2,
    ]
    lines += regions[TAIL]
    lines += [END.format(name=TAIL), "}", ""]
    return lines


HDR_DECLS = """
// W1-D: the journey/ballpark/gate crossing (the New Eden tour). The host
// sequences a tour frame (ballparkMode != OFF) itself:
//   AdvanceJourneyFrame -> [W1-C driver rungs, captureProducts 0] ->
//   RecordBallparkTelemetry, in strict order once per frame.
// AdvanceJourneyFrame is RenderFrame's pre-draw journey half (Destiny command
// issue, the journeyPhase state machine, kinematics/warp-tunnel, the
// chase-camera/solar tail, SetAnimationTime); it emits the ordered
// "Journey ..." evidence lines. RecordBallparkTelemetry is the post-draw tail
// (ballpark/celestial/eve-gate diagnostics, solar records, the sole
// ++renderedFrameCount, the shadow contract validators, the DrawFailed
// contract). Both bodies are verbatim copies of RenderFrame kept in sync by
// tools/w1d_sync_exports.py. Tour-lane invariants: freezeScene == false and
// solarParticlePrewarmCompleted == false.
//
// RecordBallparkTelemetry takes `rendered` -- the caller's driver-frame result
// (the AND of the W1-C rungs, whose ExecuteDriverProducts surfaces the same
// postprocess/distortion contract DrawDriverFrame enforces). It mirrors
// RenderFrame's `if( rendered && !freezeScene )`: on a failed draw the tail is
// skipped and renderedFrameCount does not advance.
extern "C" bool TrinityStandaloneProbeAdvanceJourneyFrame( void* opaqueProbe, int64_t realTime, int64_t simTime );
extern "C" bool TrinityStandaloneProbeRecordBallparkTelemetry( void* opaqueProbe, int64_t realTime, int64_t simTime, bool rendered );
""".rstrip("\n").split("\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--check", action="store_true",
                   help="verify the copies match RenderFrame; non-zero exit on drift")
    g.add_argument("--generate", action="store_true",
                   help="(re)build the exports from the current RenderFrame")
    args = ap.parse_args()

    if not CPP.exists():
        raise SystemExit(f"run from the repo root: {CPP} not found")

    lines = CPP.read_text().split("\n")
    regions = extract_regions(lines)
    copies = extract_copies(lines)

    if args.check:
        if copies is None:
            print("W1-D copy check: FAIL -- exports missing or lack VERBATIM COPY markers")
            return 1
        drift = False
        for name in (JOURNEY, TAIL):
            if regions[name] != copies[name]:
                drift = True
                print(f"W1-D copy check: DRIFT in the {name}")
                d = difflib.unified_diff(copies[name], regions[name],
                                         fromfile=f"export copy ({name})",
                                         tofile=f"RenderFrame ({name})", lineterm="")
                for line in list(d)[:40]:
                    print("  " + line)
            else:
                print(f"W1-D copy check: {name} in sync ({len(regions[name])} lines)")
        if drift:
            print("\nRenderFrame's journey halves changed without the exports being "
                  "regenerated.\nRun: tools/w1d_sync_exports.py --generate")
            return 1
        print("W1-D copy check: OK")
        return 0

    # --generate
    if copies is not None:
        # strip the existing export block: from the contract comment to EOF-ish
        start = next(i for i, l in enumerate(lines)
                     if l.startswith("// W1-D: the journey/ballpark/gate crossing"))
        end = next(i for i, l in enumerate(lines) if l == END.format(name=TAIL))
        # the closing brace + trailing blank of the second export
        end += 2
        lines = lines[: start - 1] + lines[end:]
        print(f"generate: replaced existing exports (removed lines {start}..{end})")

    lines = lines + [""] + build_exports(extract_regions(lines))
    CPP.write_text("\n".join(lines))
    print(f"generate: wrote {CPP} "
          f"(journey {len(regions[JOURNEY])} lines, tail {len(regions[TAIL])} lines)")

    htext = HDR.read_text()
    if "TrinityStandaloneProbeAdvanceJourneyFrame" not in htext:
        hlines = htext.split("\n")
        anchor = next(i for i, l in enumerate(hlines)
                      if "TrinityStandaloneProbeResolveAndRetainProduct(" in l)
        hlines = hlines[: anchor + 1] + HDR_DECLS + hlines[anchor + 1 :]
        HDR.write_text("\n".join(hlines))
        print(f"generate: added declarations to {HDR}")
    else:
        print(f"generate: {HDR} declarations already present")
    return 0


if __name__ == "__main__":
    sys.exit(main())
