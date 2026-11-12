"""databands.py -- the data symbols of one section range in address order, each with who references it.

The linker lays every section out in object order, so an object's globals and statics sit in one band of
each section. Listing a section's symbols with the functions that reference them shows where one
object's band ends and the next begins (a global is usually used by the object that defines it).

Usage (from tools/reverse):
  py databands.py 0190a600 0190c000                 every symbol in the range
  py databands.py 01906a00 01944b40 --fn 01851bf0 0185e9d0
        only the symbols referenced from functions in that text range, with their neighbours' users
"""
import json
import sys

import alyssapan
import symtab


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    lo, hi = int(argv[0], 16), int(argv[1], 16)
    fn = None
    if '--fn' in argv:
        i = argv.index('--fn')
        fn = (int(argv[i + 1], 16), int(argv[i + 2], 16))
    syms = sorted((e for e in symtab.entries() if e['type'] in ('OBJECT', 'NOTYPE') and lo <= e['value'] < hi
                   and not e['name'].startswith('.')), key=lambda e: (e['value'], e['index']))
    starts = [e['value'] for e in syms]
    feat = json.load(open(alyssapan.WORK / 'feat_ct3.json', encoding='utf-8'))['functions']
    users = {}
    import bisect
    for f in feat:
        for a in set(f['data']):
            i = bisect.bisect_right(starts, a) - 1
            if i >= 0 and syms[i]['value'] <= a < syms[i]['value'] + max(syms[i]['size'], 1):
                users.setdefault(i, set()).add((f['addr'], f['name']))
    for i, e in enumerate(syms):
        u = sorted(users.get(i, ()))
        if fn and not any(fn[0] <= a < fn[1] for a, _ in u):
            continue
        names = ','.join(f'{n}@{a:08x}' for a, n in u[:4]) + ('...' if len(u) > 4 else '')
        print(f'{e["value"]:08x} {e["size"]:6} {e["bind"][0]} {e["index"]:5} {e["name"]:<32} {names}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
