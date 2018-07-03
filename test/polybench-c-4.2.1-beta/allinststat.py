#!/usr/bin/env python3

import sys
import math
import os
import resource
import argparse
import itertools
import re


def execute(command, capture_stdout=False):
  import subprocess
  limit = '65532' if os.uname()[0] == 'Darwin' else 'unlimited'
  rcmd = 'ulimit -s ' + limit + '; ' + command
  if capture_stdout:
    outchan = subprocess.PIPE
  else:
    outchan = subprocess.DEVNULL
  pr = subprocess.run(rcmd, stdout=outchan, stdin=subprocess.DEVNULL, shell=True, universal_newlines=True)
  if pr.returncode == -2:
    vprint('sigint! stopping')
    os.exit(0)
  if capture_stdout:
    return pr.stdout if pr.returncode == 0 else ''
  return pr.returncode
  
  
def ParseInstrStatFile(filename, raw=False):
  out = {}
  with open(filename, 'r') as f:
    l = f.readline()
    while l != '':
      items = l.split()
      if len(items) >= 2:
        out[items[0]] = int(items[1])
      l = f.readline()
  
  if raw:
    return out, out['*']
    
  out2 = {}
  total = out['*']
  for k, v in out.items():
    out2[k] = out[k] / total
  return out2, total
  
  
def ParseLLVMStatsFile(filename, totalfl, totalfx, raw=False):
  outfl, outfx = {}, {}
  namekey = {
    "Number of instructions not replaced by a fixed-point-native equivalent": (True, True, "Fallback")
  }  
  with open(filename, 'r') as f:
    l = f.readline()
    while l != '':
      items = re.match(' *([0-9]+) +([a-z]+) *- (.*)', l)
      if items:
        orgname = items.group(3)
        if orgname in namekey:
          infl, infx, name = namekey.get(orgname)
          if infl: outfl[name] = int(items.group(1))
          if infx: outfx[name] = int(items.group(1))
      l = f.readline()
  
  if raw:
    return outfl, outfx
    
  outfl2, outfx2 = {}, {}
  for k, v in outfl.items():
    outfl2[k] = outfl[k] / totalfl
  for k, v in outfx.items():
    outfx2[k] = outfx[k] / totalfx
  return outfl2, outfx2
  
  
def MaterializeTable(iterableofiterables):
  tab = []
  sizes = []
  for iterable in iterableofiterables:
    row = []
    for item, i in zip(iterable, itertools.count()):
      stritem = str(item)
      row += [stritem]
      if len(sizes) <= i:
        sizes += [len(stritem)]
      else:
        sizes[i] = max(sizes[i], len(stritem))
    tab += [row]

  for row in tab:
    print('  '.join([item.rjust(size) for item, size in zip(row, sizes)]))
    
    
def TableGenerator(stats, allinst, datafmt, labelmod=lambda x:x):
  instlist = sorted(list(allinst))
  firstrow = ['']
  firstrow += [labelmod(inst) for inst in instlist]
  yield firstrow
  benchs = sorted(stats.keys())
  for bench in benchs:
    row = [bench]
    for inst in instlist:
      stat = stats[bench].get(inst)
      row += [datafmt(stat)]
    yield row
  
  
parser = argparse.ArgumentParser(description='instruction frequency tool')
parser.add_argument('--only-fix', action='store_true', dest='fixonly',
                    help='only fixed point benchs')
parser.add_argument('--only-flo', action='store_true', dest='floonly',
                    help='only floating point benchs')
parser.add_argument('--delta', action='store_true',
                    help='difference between float and fix')
parser.add_argument('--raw', action='store_true',
                    help='instruction counts instead of percentages')
args = parser.parse_args()
  
benchs = execute(command='./compile_everything.sh --dump-bench-list', capture_stdout=True)
benchnames = benchs.split()

stats = {}
allinst = set()
for bench in benchnames:
  fix, totfx = ParseInstrStatFile('./stats/' + bench + '_out_ic_fix.txt', raw=args.raw)
  flo, totfl = ParseInstrStatFile('./stats/' + bench + '_out_ic_float.txt', raw=args.raw)
  fl2, fx2 = ParseLLVMStatsFile('./stats/' + bench + '_out.txt', totfl, totfx, raw=args.raw)
  flo.update(fl2)
  fix.update(fx2)
  if not args.delta:
    if not args.floonly:
      stats[bench+'_fix'] = fix
    if not args.fixonly:
      stats[bench+'_flo'] = flo
  else:
    bothkeys = set(fix.keys()) | set (flo.keys())
    diff = {}
    for key in bothkeys:
      fixv = fix.get(key) or 0.0
      flov = flo.get(key) or 0.0
      diff[key] = fixv - flov
    stats[bench+'_delta'] = diff
  allinst |= set(fix.keys())
  allinst |= set(flo.keys())

if not args.raw:
  table = TableGenerator(stats, allinst - set(['*']), \
    lambda stat: '0.0' if stat is None else '%.1f'%(stat*100),
    labelmod=lambda lab:lab + ' [%]');
else:
  table = TableGenerator(stats, allinst, \
    lambda stat: '0' if stat is None else '%d'%stat)
MaterializeTable(table)
  
  
  
  
  
  
