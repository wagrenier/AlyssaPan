"""CT3's TU map from its symtab (bootstrap 5, 2026-11-09): one game object file per run of local symbols.

The symtab has no FILE symbols, but ELF local symbols are emitted per input object in link order, and each
object's data lands contiguously, in that same order, in every output subsection. Within an object MW
writes its locals in hash order, so a single object is not sorted -- but no object's locals interleave
another's. Three steps:

1. Subsections. The coarse regions of data/layout.json mix MW's subsections (.rodata, .sdata, .sbss sit in
   one region). Sorted by address, the locals' symtab index restarts at each subsection start: those
   restarts are the subsection bounds (CT3: .data, .rodata ..0x01937610, .sdata ..0x0193bc80, .sbss
   ..0x01944b00, .bss).
2. Chunks. A cut between locals i-1 and i is possible only where, in every subsection, every earlier
   local's address is below every later one's. Real object bounds are a subset of the possible cuts, so a
   chunk (between two possible cuts) is one object or part of one; adjacent chunks may be one object.
3. Functions. Each game function gets the chunk of the median local it references (statics, `@<n>`
   literals, static functions; work/feat_ct3.json). Text order must follow chunk order: the longest
   non-decreasing run of keys keeps the evidence, the rest (feature false positives) is dropped. Runs of
   one key become objects; adjacent runs merge when two functions reference both chunks. Unkeyed functions
   between two objects go with the earlier one (`gap` in the report: the TU session checks them).

File names: the source strings (`ahalloc.c`, `movie.c`, `fileio.c`, `sound.c`, `xcmd_tn.c`) where a
function names one, else the functions' common prefix, else the first function; `ga/` for the `ga*`
library range, `game/` for the rest. Ranges an existing TU in data/tus.json already owns are left alone
(game/main.c).

    python tumap.py                 # report (work/tumap_ct3.json)
    python tumap.py --apply         # write every object into data/tus.json ('why' = the evidence), then export.py
"""
import bisect
import json
import os
import statistics
import subprocess
import sys

import alyssapan
import symtab

# Game text (architecture.md "Address map"); the SDK, newlib, libgcc and sdremote ranges are host code.
GAME_TEXT = [(0x01834440, 0x0183aed8), (0x0183d9e0, 0x018d2a68), (0x018d2ee0, 0x018fe500)]
GA_LIB = (0x0189f718, 0x018a8810)
SOURCE_FILES = ('ahalloc.c', 'movie.c', 'fileio.c', 'sound.c', 'xcmd_tn.c')
IMAGE = (0x01810000, 0x01c8d780)


def subsections(locs):
    """[(lo, hi)] of the output subsections, from where the index restarts in address order."""
    data = sorted((e['value'], e['index']) for e in locs if e['type'] == 'OBJECT')
    cuts = [IMAGE[0], 0x018fe500, 0x01906a00]      # text, .vutext, data start (the layout's own)
    for (a0, i0), (a1, i1) in zip(data, data[1:]):
        if i0 - i1 > 400:
            cuts.append(a1)
    cuts = sorted(set(cuts)) + [IMAGE[1]]
    return list(zip(cuts, cuts[1:]))


def chunks(locs, subs):
    """Possible object cuts: [(first, last)] positions into locs."""
    lows = [lo for lo, _ in subs]
    sid = [bisect.bisect_right(lows, e['value']) - 1 for e in locs]
    n, k = len(locs), len(subs)
    pmax = [[-1] * (n + 1) for _ in range(k)]
    smin = [[1 << 40] * (n + 1) for _ in range(k)]
    for s in range(k):
        m = -1
        for i in range(n):
            pmax[s][i] = m
            if sid[i] == s:
                m = max(m, locs[i]['value'])
        m = 1 << 40
        for i in range(n - 1, -1, -1):
            if sid[i] == s:
                m = min(m, locs[i]['value'])
            smin[s][i] = m
    cuts = [i for i in range(1, n) if all(pmax[s][i] < smin[s][i] for s in range(k))]
    b = [0] + cuts + [n]
    return list(zip(b, [x - 1 for x in b[1:]]))


def lnds(keys):
    """Indices of a longest non-decreasing subsequence of keys."""
    tails, tail_i, prev = [], [], [-1] * len(keys)
    for i, k in enumerate(keys):
        j = bisect.bisect_right(tails, k)
        if j == len(tails):
            tails.append(k)
            tail_i.append(i)
        else:
            tails[j] = k
            tail_i[j] = i
        prev[i] = tail_i[j - 1] if j else -1
    out, i = [], tail_i[-1] if tail_i else -1
    while i >= 0:
        out.append(i)
        i = prev[i]
    return out[::-1]


def common_prefix(names):
    if not names:
        return ''
    p = os.path.commonprefix(names)
    while p and not p[-1].isalnum():
        p = p[:-1]
    return p


def build():
    syms = symtab.entries()
    locs = [e for e in syms if e['bind'] == 'LOCAL' and e['type'] in ('OBJECT', 'FUNC')
            and IMAGE[0] <= e['value'] < IMAGE[1]]
    subs = subsections(locs)
    ch = chunks(locs, subs)
    chunk_of = {}
    for c, (a, z) in enumerate(ch):
        for i in range(a, z + 1):
            chunk_of[locs[i]['index']] = c
    objs = sorted((e['value'], e['value'] + max(e['size'], 1), e['index']) for e in locs if e['type'] == 'OBJECT')
    starts = [o[0] for o in objs]
    lfunc = {e['value']: e['index'] for e in locs if e['type'] == 'FUNC'}

    def local_at(a):
        i = bisect.bisect_right(starts, a) - 1
        return objs[i][2] if i >= 0 and objs[i][0] <= a < objs[i][1] else None

    feat = json.load(open(alyssapan.WORK / 'feat_ct3.json', encoding='utf-8'))['functions']
    funcs = sorted((f for f in feat if any(lo <= f['addr'] < hi for lo, hi in GAME_TEXT)), key=lambda f: f['addr'])
    for f in funcs:
        t = {local_at(a) for a in f['data']} | {lfunc.get(a) for a in f['calls'] + f['tails'] + f['fptrs']}
        t.add(lfunc.get(f['addr']))
        t.discard(None)
        f['chunks'] = sorted(chunk_of[i] for i in t if i in chunk_of)
        f['src'] = next((s for s in SOURCE_FILES if s in f['strings']), None)
    keyed = [f for f in funcs if f['chunks']]
    keep = set(lnds([statistics.median_low(f['chunks']) for f in keyed]))
    for i, f in enumerate(keyed):
        f['key'] = statistics.median_low(f['chunks']) if i in keep else None
    # runs of one key, in text order
    runs = []
    for f in funcs:
        k = f.get('key')
        if k is None:
            continue
        if runs and runs[-1]['keys'][-1] == k:
            runs[-1]['funcs'].append(f)
        else:
            runs.append({'keys': [k], 'funcs': [f]})
    # merge adjacent runs when two functions reference both chunks
    merged = []
    for r in runs:
        if merged:
            p = merged[-1]
            a, b = set(p['keys']), set(r['keys'])
            both = sum(1 for f in p['funcs'] + r['funcs'] if a & set(f['chunks']) and b & set(f['chunks']))
            if both >= 2:
                p['keys'] += r['keys']
                p['funcs'] += r['funcs']
                continue
        merged.append(r)
    # extents: from the first keyed function to the next object's first; section ends cut too
    out = []
    addrs = [f['addr'] for f in funcs]
    # Cuts: the game text sections' starts, the .text section symbols (asm / GCC objects start there), and
    # one cut in each gap between two runs, where the names on each side match their run's names best.
    sect = {e['value'] for e in syms if e['type'] == 'SECTION' and e['name'] == '.text'
            and any(lo <= e['value'] < hi for lo, hi in GAME_TEXT)}
    cuts = {lo for lo, _ in GAME_TEXT} | sect

    def sim(name, run):
        return max(len(os.path.commonprefix([name, f['name']])) for f in run['funcs'])

    for a, b in zip(merged, merged[1:]):
        last, first = a['funcs'][-1]['addr'], b['funcs'][0]['addr']
        gap = [f for f in funcs if last < f['addr'] < first]
        if any(c for c in sect if last < c <= first):
            cuts.add(first)       # a section symbol starts an asm / GCC object: no MW statics, so b starts its own
            continue
        if any(c for c in cuts if last < c <= first):
            continue              # a game section's start already separates them
        best = max(range(len(gap) + 1), key=lambda p: (sum(sim(f['name'], a) for f in gap[:p])
                                                        + sum(sim(f['name'], b) for f in gap[p:]), -p))
        cuts.add(gap[best]['addr'] if best < len(gap) else first)
    bounds = sorted(cuts | {hi for _, hi in GAME_TEXT})
    for lo, hi in zip(bounds, bounds[1:]):
        if not any(s[0] <= lo < s[1] for s in GAME_TEXT):
            continue                                          # between two game sections (libgcc, sdremote)
        members = [f for f in funcs if lo <= f['addr'] < hi]
        if not members:
            continue
        keys = sorted({f['key'] for f in members if f.get('key') is not None})
        out.append({'lo': lo, 'hi': hi, 'chunks': keys,
                    'functions': [f['name'] for f in members],
                    'evidence': sum(1 for f in members if f.get('key') is not None),
                    'gap': [f['name'] for f in members if not f['chunks']],
                    'src': sorted({f['src'] for f in members if f['src']})})
    return {'subsections': [[f'{a:08x}', f'{b:08x}'] for a, b in subs], 'chunks': len(ch), 'objects': out,
            'dropped': [f['name'] for f in keyed if f['key'] is None]}


def key_for(o, taken):
    d = 'ga' if GA_LIB[0] <= o['lo'] < GA_LIB[1] else 'game'
    if len(o['src']) == 1:
        stem = o['src'][0][:-2]
    else:
        names = [n for n in o['functions'] if not n.startswith('FUN_')]
        stem = common_prefix(names)
        if len(stem) < 4:
            stem = names[0] if names else f'obj_{o["lo"]:08x}'
    stem = stem.strip('_') or f'obj_{o["lo"]:08x}'
    key = f'{d}/{stem}.c'
    n = 2
    while key in taken:
        key = f'{d}/{stem}_{n}.c'
        n += 1
    taken.add(key)
    return key


def main(argv):
    apply = '--apply' in argv
    m = build()
    owned = alyssapan.tu_ranges()
    db = alyssapan.load_json(alyssapan.TUS_JSON, {})
    taken = set(db.get('tus', {}))
    for o in m['objects']:
        clash = [k for lo, hi, k in owned if lo < o['hi'] and o['lo'] < hi]
        o['owner'] = clash[0] if clash else None
        o['key'] = clash[0] if clash else key_for(o, taken)
    (alyssapan.WORK / 'tumap_ct3.json').write_text(json.dumps(m, indent=1), encoding='utf-8')
    print(f"subsections {len(m['subsections'])}, chunks {m['chunks']}, objects {len(m['objects'])}, "
          f"dropped as outliers {len(m['dropped'])}")
    for o in m['objects']:
        print(f"{o['lo']:08x}-{o['hi']:08x} {len(o['functions']):4d} fn ({o['evidence']:3d} keyed, {len(o['gap']):3d} gap) "
              f"{o['key']}{' (already defined)' if o['owner'] else ''}  {o['functions'][0]} .. {o['functions'][-1]}")
    if not apply:
        return 0
    tus = db.setdefault('tus', {})
    n = 0
    for o in m['objects']:
        if o['owner']:
            continue
        tus[o['key']] = {'ranges': [[f"{o['lo']:08x}", f"{o['hi']:08x}"]],
                         'why': ((f"tumap.py (bootstrap 5): symtab local chunks {o['chunks'][0]}..{o['chunks'][-1]}, "
                                  if o['chunks'] else "tumap.py (bootstrap 5): a .text section symbol starts it (asm or GCC object), ")
                                 + f"{o['evidence']} functions keyed by their statics, {len(o['gap'])} unkeyed"
                                 + (f"; source string {', '.join(o['src'])}" if o['src'] else '; file name from the names'))}
        n += 1
    alyssapan.TUS_JSON.write_text(json.dumps(db, indent=1, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f'{n} objects written to data/tus.json')
    return subprocess.run([sys.executable, 'export.py']).returncode


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
