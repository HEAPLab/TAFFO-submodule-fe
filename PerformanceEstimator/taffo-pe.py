#!/usr/bin/env python2
import argparse


def load_model(fn):
	from joblib import load
	k = load(fn)
	print 'loaded model', k['model_name']
	return k['fitr'], k['features']


if __name__=='__main__' :
	import os

	try:
		mlfeatpath = os.environ['TAFFO_MLFEAT']
	except:
		mlfeatpath = None
	parser = argparse.ArgumentParser()
	parser.add_argument('pred', help='input set', nargs='?', default='./data/20190623_polybench_antarex/')
	parser.add_argument('--fix', '-f', type=str, help='fixed point .ll file', required=True)
	parser.add_argument('--flt', '-F', type=str, help='floating point .ll file', required=True)
	parser.add_argument('--mlfeat-path', help='path to taffo-mlfeat', required=mlfeatpath is None, default=mlfeatpath)
	parser.add_argument('--model', '-m', type=str, help='model file', default='saved_model.bin')
	args = parser.parse_args()

	import subprocess
	from StringIO import StringIO
	from stats_loader import *
	fix_feat_f = subprocess.check_output([args.mlfeat_path + ' "' + args.fix + '"'], shell=True)
	fix_d = load_one_stats_new(StringIO(fix_feat_f), 'tag', 'bench', True)
	flt_feat_f = subprocess.check_output([args.mlfeat_path + ' "' + args.flt + '"'], shell=True)
	flt_d = load_one_stats_new(StringIO(fix_feat_f), 'tag', 'bench', False)

	fitr, features = load_model(args.model)

	from classification import *
	test, _ = load_data_onlystats([fix_d, flt_d], features)

	pred = fitr.predict(test.loc[:, features])
	print 'Predict', pred[0]



