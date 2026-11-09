"""CT3's own .symtab: read it, and apply it where Ghidra's ELF loader left gaps (bootstrap 4, 2026-11-09).

Clock Tower 3 (SLUS_206.33) keeps its full symbol table (architecture.md "Binary and toolchain"): 6,179
entries, with every game function's name and MW's local labels (`@<n>` literals, `<name>$<n>` statics) in
link order. These names are the game's own: tag `game-symbol`, never renamed.

    python symtab.py list [--json]       # the table: counts by bind / type, or every entry (work/symtab_ct3.json)
    python symtab.py tags [--apply]      # every FUNC symbol that starts a Ghidra function -> game-symbol
                                         # (data/tags.json, and Ghidra's function tags in one transaction)
    python symtab.py missing [--apply]   # FUNC symbols with no Ghidra function start -> create_function
    python symtab.py labels [--apply]    # OBJECT / NOTYPE symbols without a Ghidra label of that name -> create_label

Without --apply each command only reports. The TU map (bootstrap 5) will read the same table (local-symbol
order): `entries()` is the reader for it.
"""
import argparse
import json
import struct
import sys

import alyssapan

BIND = {0: 'LOCAL', 1: 'GLOBAL', 2: 'WEAK'}
TYPE = {0: 'NOTYPE', 1: 'OBJECT', 2: 'FUNC', 3: 'SECTION', 4: 'FILE'}
SHN_ABS, SHN_COMMON = 0xfff1, 0xfff2


def entries(path=None):
    """[{'index', 'name', 'value', 'size', 'bind', 'type', 'shndx'}] in table order (index 0 skipped)."""
    d = open(path or alyssapan.ELF, 'rb').read()
    (shoff,) = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, _ = struct.unpack_from('<HHH', d, 0x2e)
    secs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    symtab = next(s for s in secs if s[1] == 2)                 # SHT_SYMTAB
    strtab = secs[symtab[6]]                                    # sh_link -> its string table
    out = []
    for i in range(1, symtab[5] // 16):
        name_off, value, size, info, _, shndx = struct.unpack_from('<IIIBBH', d, symtab[4] + i * 16)
        end = d.index(b'\0', strtab[4] + name_off)
        out.append({'index': i, 'name': d[strtab[4] + name_off:end].decode('ascii', 'replace'),
                    'value': value, 'size': size, 'bind': BIND.get(info >> 4, str(info >> 4)),
                    'type': TYPE.get(info & 15, str(info & 15)), 'shndx': shndx})
    return out


def functions(syms):
    """{addr: name} of the FUNC symbols (the first name wins where two share an address)."""
    out = {}
    for s in syms:
        if s['type'] == 'FUNC' and s['name'] and s['value']:
            out.setdefault(s['value'], s['name'])
    return out


def ghidra_functions():
    """{addr: name} of Ghidra's function starts: live when Ghidra answers, else work/hash_ct3.json."""
    if alyssapan.ghidra_up():
        return {int(a, 16): n for a, n in alyssapan.function_names().items()}
    fs = alyssapan.load_json(alyssapan.WORK / 'hash_ct3.json', {}).get('functions', [])
    return {int(f['address'], 16): f['name'] for f in fs}


def cmd_list(args, syms):
    from collections import Counter
    c = Counter((s['bind'], s['type']) for s in syms)
    for k, n in sorted(c.items()):
        print(f'{k[0]:6} {k[1]:7} {n}')
    print(f'{len(syms)} entries')
    if args.json:
        alyssapan.save_json(alyssapan.WORK / 'symtab_ct3.json', syms)
        print('-> work/symtab_ct3.json')


def cmd_tags(args, syms):
    funcs = functions(syms)
    gf = ghidra_functions()
    # Hand-written objects (entry, trans_for_gs, the gaDrawObject*_Redraw_Polygon* asm) name their entry
    # points with NOTYPE labels, not FUNC symbols: a Ghidra function carrying such a label's name is the
    # game's own name too.
    for s in syms:
        if s['type'] == 'NOTYPE' and s['value'] in gf and s['value'] not in funcs and gf[s['value']] == s['name']:
            funcs[s['value']] = s['name']
    if 0x01810008 in gf and gf[0x01810008] == 'entry':
        funcs.setdefault(0x01810008, 'entry')       # Ghidra's name for ENTRYPOINT / _start: keep, tag as symbol
    tags = alyssapan.tags()
    todo, renamed = [], []
    for a, name in sorted(funcs.items()):
        if a not in gf:
            continue                                            # `missing` creates it first
        if gf[a] != name:
            renamed.append((a, gf[a], name))
        if tags.get(f'{a:08x}') != 'game-symbol':
            todo.append(a)
    print(f'{len(funcs)} FUNC symbols, {len(todo)} to tag game-symbol; '
          f'{len(renamed)} where Ghidra shows another name')
    for a, g, n in renamed[:20]:
        print(f'  {a:08x} Ghidra {g!r}, symtab {n!r}')
    if not args.apply:
        return
    for a, g, n in renamed:                                     # the loader demangled a C name: restore it
        print(f'  restore {a:08x} {n}:', alyssapan.rename(a, n) or 'ok')
    if not todo:
        print(alyssapan.save_program() if renamed else 'nothing to tag')
        return
    for a in todo:
        tags[f'{a:08x}'] = 'game-symbol'
    alyssapan.save_json(alyssapan.TAGS_JSON, tags)
    print(f'data/tags.json: {len(tags)} tags')
    if alyssapan.ghidra_up():
        for i in range(0, len(todo), 500):
            chunk = todo[i:i + 500]
            alyssapan.http('POST', 'add_function_tag', {
                'assignments': [{'function': f'0x{a:x}', 'tags': 'game-symbol'} for a in chunk],
                'tag_comments': {'game-symbol': alyssapan.TAG_COMMENTS['game-symbol']}}, timeout=600)
        print('Ghidra function tags:', len(todo), alyssapan.save_program())


def cmd_missing(args, syms):
    funcs = functions(syms)
    gf = ghidra_functions()
    miss = [(a, n) for a, n in sorted(funcs.items()) if a not in gf]
    print(f'{len(miss)} FUNC symbols without a Ghidra function start')
    for a, n in miss:
        print(f'  {a:08x} {n}')
    extra = [(a, n) for a, n in sorted(gf.items()) if a not in funcs]
    print(f'{len(extra)} Ghidra functions without a FUNC symbol (split bodies, code the symtab names as a label):')
    labels = {}
    for s in syms:
        if s['value'] and s['type'] != 'SECTION':
            labels.setdefault(s['value'], []).append(s['name'])
    for a, n in extra:
        print(f'  {a:08x} {n}  symtab: {", ".join(labels.get(a, [])) or "-"}')
    if args.apply and miss:
        made = [a for a, n in miss if alyssapan.create_function(a, n)]
        tags = alyssapan.tags()
        for a in made:
            tags[f'{a:08x}'] = 'game-symbol'
        alyssapan.save_json(alyssapan.TAGS_JSON, tags)
        print(f'created {len(made)} of {len(miss)};', alyssapan.save_program())


def label_symbols(syms):
    """OBJECT / NOTYPE symbols in the image worth a label: named, inside `main` or .bss, not section
    markers. MW's `@<n>` literals and `<name>$<n>` statics are the ones Ghidra's loader may have dropped."""
    out = []
    for s in syms:
        if s['type'] in ('OBJECT', 'NOTYPE') and s['name'] and s['value'] >= 0x01810000 \
                and s['shndx'] not in (0, SHN_ABS) and not s['name'].startswith('.') \
                and s['name'] not in ('gcc2_compiled.', '__gnu_compiled_c'):
            out.append(s)
    return out


def cmd_labels(args, syms):
    """create_label skips a label that already exists (same name, same address), so the batch itself is
    the comparison: list_globals shows only ~200 of them (it leaves out labels without xrefs and code
    labels), it is no measure of what the loader applied."""
    ls = label_symbols(syms)
    from collections import Counter
    kind = Counter('@literal' if s['name'].startswith('@') else 'static$' if '$' in s['name'] else 'named'
                   for s in ls)
    print(f'{len(ls)} data / code-label symbols in the image: {dict(kind)}')
    if not args.apply:
        return
    if not alyssapan.ghidra_up():
        print('Ghidra down: nothing applied')
        return
    total = Counter()
    for i in range(0, len(ls), 500):
        d = alyssapan.http('POST', 'create_label', {'labels': [{'address': f'0x{s["value"]:x}', 'name': s['name']}
                                                            for s in ls[i:i + 500]]}, timeout=600)
        for k in ('labels_created', 'labels_skipped', 'labels_failed'):
            total[k] += int(d.get(k) or 0)
        for e in (d.get('errors') or [])[:5]:
            print('  ', e)
    print(dict(total))
    if total['labels_created']:
        print(alyssapan.save_program())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('cmd', choices=('list', 'tags', 'missing', 'labels'))
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--json', action='store_true')
    args = ap.parse_args()
    syms = entries()
    {'list': cmd_list, 'tags': cmd_tags, 'missing': cmd_missing, 'labels': cmd_labels}[args.cmd](args, syms)


if __name__ == '__main__':
    sys.exit(main())
