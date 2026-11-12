"""seams.py -- evidence for splitting a TU map object into its real objects.

tumap.py cut the game where a local symbol showed a seam; objects with few statics came out too large.
This lists, for the functions of one address range in text order, what ties each to its neighbours:

  - the MW literals it references (@<n>) and the statics (<name>$<n>): CodeWarrior numbers both from one
    counter per compilation unit, so the numbers grow through an object and drop (restart low) at the
    next one;
  - the symtab index of every local it references (an object's locals sit in one run of indices);
  - static (LOCAL FUNC) callees and callers: a static function lives in its callers' object;
  - callers from outside the range (who calls a run tells which module it serves).

Usage (from tools/reverse):
  py seams.py 01851bf0 0185e9d0            one line per function
  py seams.py 01851bf0 0185e9d0 --runs     group functions whose numbers keep growing; print the drops
"""
import bisect
import json
import re
import sys

import alyssapan
import symtab

IMAGE = (0x01810000, 0x02000000)


def load():
    syms = symtab.entries()
    locs = [e for e in syms if e['bind'] == 'LOCAL' and e['type'] in ('OBJECT', 'FUNC')
            and IMAGE[0] <= e['value'] < IMAGE[1]]
    objs = sorted((e['value'], e['value'] + max(e['size'], 1), e) for e in locs if e['type'] == 'OBJECT')
    lfunc = {e['value']: e for e in locs if e['type'] == 'FUNC'}
    feat = json.load(open(alyssapan.WORK / 'feat_ct3.json', encoding='utf-8'))['functions']
    return objs, lfunc, sorted(feat, key=lambda f: f['addr'])


def counter(name):
    """The MW counter of a local: @123 -> 123, name$45 -> 45, else None."""
    m = re.fullmatch(r'@(\d+)', name) or re.fullmatch(r'.+\$(\d+)', name)
    return int(m.group(1)) if m else None


def analyse(lo, hi):
    objs, lfunc, feat = load()
    starts = [o[0] for o in objs]

    def local_at(a):
        i = bisect.bisect_right(starts, a) - 1
        return objs[i][2] if i >= 0 and objs[i][0] <= a < objs[i][1] else None

    byaddr = {f['addr']: f for f in feat}
    inside = [f for f in feat if lo <= f['addr'] < hi]
    callers = {}
    for f in feat:
        for c in set(f['calls'] + f['tails'] + f['fptrs']):
            callers.setdefault(c, set()).add(f['addr'])
    rows = []
    for f in inside:
        locs = {}
        for a in f['data']:
            e = local_at(a)
            if e:
                locs[e['index']] = e
        nums = sorted(n for n in (counter(e['name']) for e in locs.values()) if n is not None)
        statics = [lfunc[c]['name'] for c in set(f['calls'] + f['tails'] + f['fptrs']) if c in lfunc
                   and lo <= c < hi]
        out = sorted({byaddr[c]['name'] for c in callers.get(f['addr'], ()) if not lo <= c < hi
                      and c in byaddr})
        rows.append({'addr': f['addr'], 'name': f['name'], 'static': f['addr'] in lfunc,
                     'nums': nums, 'idx': sorted(locs), 'names': sorted(e['name'] for e in locs.values()),
                     'statics': statics, 'ext': out})
    return rows


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    lo, hi = int(argv[0], 16), int(argv[1], 16)
    rows = analyse(lo, hi)
    if '--runs' in argv:
        # a drop: a function's smallest number below the largest number seen since the last drop
        run, top = [], -1
        for r in rows:
            if r['nums'] and top >= 0 and r['nums'][0] < top - 0:
                print(f'--- drop at {r["addr"]:08x} {r["name"]}: {r["nums"][0]} after {top}')
                top = -1
            if r['nums']:
                top = max(top, r['nums'][-1])
        return 0
    for r in rows:
        tag = 's' if r['static'] else ' '
        nums = ','.join(map(str, r['nums']))
        idx = f'{r["idx"][0]}..{r["idx"][-1]}' if r['idx'] else ''
        print(f'{r["addr"]:08x} {tag} {r["name"]:<40} #[{nums}] idx[{idx}]'
              + (f' calls-static {r["statics"]}' if r['statics'] else '')
              + (f' <- {",".join(r["ext"][:4])}{"..." if len(r["ext"]) > 4 else ""}' if r['ext'] else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
