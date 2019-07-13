#!/usr/bin/python
__doc__ = '''Load the compiler stats data from multiple files, dump as CSV (standalone) or return Dataframe (as library)'''
from commands import getoutput as cmd
from string import split, strip
import pandas as pd
import numpy as np
import os


def load_file_stats(fname):
	d = pd.read_csv(fname,sep='\s+', index_col=0, header=None, engine='python')
	d=d.transpose()
	names = split(fname,'/')
	names = split(names[-1],'_')[0]
	d['Tag']= 'good' if 'good' in fname else ('bad' if 'bad' in fname else 'worse')
	d['Bench']=names
	d['Out'] = 'fix' if 'fix' in fname else 'float'
	d=d.set_index('Tag',append=False)
	d=d.set_index('Bench',append=True)
	d=d.set_index('Out',append=True)
	return d

def load_file_stats_new(fname):
	d = pd.read_csv(fname,sep='\s+', index_col=0, header=None, engine='python')
	d=d.transpose()
	names = split(fname,'/')
	names = split(names[-1],'.')[0]
	tag = os.path.basename(os.path.dirname(fname))
	d['Tag']= tag
	d['Bench']=names
	d['Out'] = 'float' if 'float' in fname else 'fix'
	d=d.set_index('Tag',append=False)
	d=d.set_index('Bench',append=True)
	d=d.set_index('Out',append=True)
	return d

def load_stats(path='./'):	
	file_list = split(cmd('ls '+path+'*-*/*_ic_*'))
	if len(file_list) > 0 and os.path.exists(file_list[0]):
		# OLD DIRECTORY FORMAT
		file_list = [ x for x in file_list if 'raw' not in x ]
		files = [load_file_stats(fname) for fname in file_list ]
	else:
		file_list = split(cmd('ls ' + path + '*/*.mlfeat.txt'))
		files = [load_file_stats_new(fname) for fname in file_list]
	d = pd.concat(files)
	d = d.unstack(level='Out')
	d = d.swaplevel('Tag','Bench')
	return d

if __name__=='__main__':
	#d=load_stats('./20180911_multiconf_results/')
	d = load_stats('./20190620_polybench/')
	print d
	d.to_csv(path_or_buf="stats.csv")
