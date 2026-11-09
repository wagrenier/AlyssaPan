// common/alyssapan_stub.h -- marks a function reconstructed code calls but nobody has written yet. The body
// logs once (name + CT3 address) and returns, so the build links and a run shows exactly which missing
// pieces were reached. tools/reverse/stubgen.py writes these, status.py counts them; replace the whole
// definition when the function is reconstructed.
#pragma once

#include <stdio.h>

#define ALYSSAPAN_STUB(name, ct3_addr)                                                                     \
    do {                                                                                                 \
        static int alyssapan_stub_seen_;                                                                   \
        if (!alyssapan_stub_seen_) {                                                                       \
            alyssapan_stub_seen_ = 1;                                                                      \
            printf("[stub] %s (CT3 0x%08x) reached\n", name, (unsigned)(ct3_addr));                      \
        }                                                                                                \
    } while (0)

// A temporary stand-in that works but is not CT3's code (host-side behaviour so later TUs can run). The
// tools count it exactly like a stub: still to reconstruct, listed in its TU's brief.
#define ALYSSAPAN_BRIDGE(name, ct3_addr)                                                                   \
    do {                                                                                                 \
        static int alyssapan_stub_seen_;                                                                   \
        if (!alyssapan_stub_seen_) {                                                                       \
            alyssapan_stub_seen_ = 1;                                                                      \
            printf("[stub] %s (CT3 0x%08x) reached (bridge)\n", name, (unsigned)(ct3_addr));             \
        }                                                                                                \
    } while (0)
