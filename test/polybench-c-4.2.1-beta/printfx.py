#!/usr/bin/env python3

# This thing exists solely because GNU printf(2) is retarded does not
# accept positional parameters
#
# Message to the a*hole who intentionally omitted positional parameters from
# GNU printf: there are no security implications because you can check your
# f*ing argc, goddammit! Ah, the same message holds for the a*hole who did the
# same for python printf formatting

import sys
import re

pos = re.findall('\%([0-9]+\$)?[^%]', sys.argv[1])
idxs = []
for i in range(len(pos)):
  if len(pos[i]) > 0:
    idxs.append(int(pos[i][0:-1]))
  else:
    idxs.append(i+1)
par = [sys.argv[i+1] for i in idxs]
fmt = re.sub('(\%)[0-9]+\$([^%])', '\\1\\2', sys.argv[1])
fmtd = bytes(fmt, "utf-8").decode("unicode_escape")

print(fmtd % tuple(par), end='')

