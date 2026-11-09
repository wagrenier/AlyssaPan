"""Decompile functions over GhidraMCP: python dec.py <addr|name> [...]"""
import sys

import alyssapan

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

for target in sys.argv[1:]:
    print(f'===== {alyssapan.PROGRAM} {target}')
    print(alyssapan.decompile(target))
    print()
