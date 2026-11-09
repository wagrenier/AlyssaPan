"""Dump what the offline tools need from Ghidra into work/.

python fetch.py

Writes work/hash_ct3.json (every function's start, name, instruction count and normalized opcode hash,
from GhidraMCP's paged get_function_hash) and work/ct3_tags.json (the provenance tag of every tagged
function). Read-only. Tags come from data/tags.json, the record the naming tools keep
(alyssapan.set_tag), so they need no Ghidra.
"""
import alyssapan

funcs = [{'address': f['address'], 'name': f['name'], 'hash': f.get('hash', ''),
          'instruction_count': f.get('instruction_count', 0)}
         for f in alyssapan.paged('get_function_hash', 'functions')]
alyssapan.WORK.mkdir(parents=True, exist_ok=True)
alyssapan.save_json(alyssapan.WORK / 'hash_ct3.json', {'functions': funcs}, indent=None)
print(f'{alyssapan.PROGRAM}: {len(funcs)} functions')

tags = {a: t for a, t in alyssapan.tags().items() if t in alyssapan.ALL_TAGS}
alyssapan.save_json(alyssapan.WORK / 'ct3_tags.json', tags)
print(f'{alyssapan.PROGRAM} tags: {len(tags)}')
