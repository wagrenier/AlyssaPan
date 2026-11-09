# AlyssaPan status (generated)

Generated 2026-11-09 17:15 at commit `(no commit yet)` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 0; tagged game-symbol in Ghidra: 0
- library matches planned: 0; tagged lib-match / -low in Ghidra: 0 / 0
- classes (named vtables): 0

## Naming (game code; library ranges excluded)

- game functions: **1814**, named **1807** (100%); library functions: 814
- by provenance: game-symbol 0, lib-match 0, lib-match-low 0, lib-context 0, game-inferred 0
- context-name ledger: 0 entries; overrides of wrong library matches: 0
- placed in a source file: 0/1814 in 0 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `01850000` | 197/200 (98%) | - |
| `018d0000` | 180/182 (99%) | - |
| `018b0000` | 146/147 (99%) | - |
| `01890000` | 147/148 (99%) | - |
| `01830000` | 39/39 (100%) | - |
| `01840000` | 84/84 (100%) | - |
| `01860000` | 113/113 (100%) | - |
| `01870000` | 82/82 (100%) | - |

## Reconstruction (src/)

- reconstructed CT3 functions: **0**; stubs: **1**

| file | reconstructed | stubs |
|---|---|---|
| `src/lib/unassigned_stubs.cpp` | 0 | 1 |

## Translation units (one session each: `tus.py`)

- 3 TUs: todo 3
- next: **unassigned@01834440** (frontier score 5)
- pinned queue: (empty)

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@01834440` | todo | 0/1/29 | 26.5 | 5 | 1 |

## Frontier (called by reconstructed code, not written yet)

- empty: nothing reconstructed calls an unwritten function yet. Start from `main` (crt0 `_start` calls it) or the queue in `references/open-items.md`.

