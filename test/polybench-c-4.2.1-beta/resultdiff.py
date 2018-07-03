#!/usr/bin/env python3

import sys
import math
from decimal import *


def ReadValues(filename):
  with open(filename, 'r') as f:
    l = f.readline()
    while l != '':
      for v in l.strip().split():
        if v != '':
          yield v
      l = f.readline()


n = 0
accerr = Decimal(0)
accval = Decimal(0)
fix_nofl = 0
flo_nofl = 0

thres_ofl_cp = Decimal('0.01')

for svfix, svflo in zip(ReadValues(sys.argv[1]), ReadValues(sys.argv[2])):
  vfix, vflo = Decimal(svfix), Decimal(svflo)
  if vfix.is_nan():
    fix_nofl += 1
  elif vflo.is_nan():
    flo_nofl += 1
    fix_nofl += 1
  elif ((vflo + vfix).copy_abs() - (vflo.copy_abs() + vfix.copy_abs())) > thres_ofl_cp:
    fix_nofl += 1
  else:
    n += 1
    accerr += (vflo - vfix).copy_abs()
    accval += vflo
  
print(fix_nofl, flo_nofl, \
      '%.5f' % (accerr / accval * 100) if accval > 0 and n > 0 else '-', \
      '%.5e' % (accerr / n) if n > 0 else '-')

