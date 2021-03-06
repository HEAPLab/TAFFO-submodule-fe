#!/usr/bin/env python3
__doc__ = '''Use RandomForestClassifier to classify whether Fix is faster'''
import pandas as pd
import numpy as np
from math import sqrt
from stats_loader import load_stats
from profile_loader import load_profile
import time as pytime
import argparse
from preprocess import *
from classification import *
from pe_utils import *


def main():
	from sys import argv

	parser = argparse.ArgumentParser()
	parser.add_argument('pred', help='input set', nargs='?', default='./data/20190623_polybench_antarex/')
	parser.add_argument('--train', '-t', help='training set', default='./data/20190623_polybench_antarex/')
	parser.add_argument('--test', '-T', action='store_true', help='run in test mode (use as training set random partition of training set)')
	parser.add_argument('--ntry', '-n', type=int, help='number of retries', default=100)
	parser.add_argument('--single', '-s', action='store_true', help='only predict one bench at a time')
	parser.add_argument('--boostfail', '-f', type=int, help='amplify inputs in training set with low speedup by the given factor', default=5)
	parser.add_argument('--dump', '-d', type=str, help='save the specified model to a file named saved_model.bin (specify "best" for selecting best estimator automatically)')
	args = parser.parse_args()

	#base_path='./20180911_multiconf_results/'
	base_path = args.train
	path = args.pred
	base, features, response = load_data(base_path, boostfail=args.boostfail)
	train=base
	print(path)
	
	#import warnings
	#warnings.simplefilter("error")
	test, f_2, r_2 = load_data(path, features, response)
	#print test

	estimators = [
		['Random Forest',              train_randomforest,         [], []],
		['Extremely Randomized Trees', train_extree,               [], []],
		['Bagging',                    train_bagging,              [], []],
		['AdaBoost',                   train_adaboost,             [], []],
		['Gradient Tree',              train_gradient,             [], []],
		['Multilayer Perceptron',      train_multilayerperceptron, [], []],
		['SVC',                        train_svc,                  [], []],
		#['Grid Search Best Estimator', pipeline, [], []] # takes hours and hours
	]
	n = args.ntry
	for i in range(n):
		if args.test:
			train, test = split_train_test(base)
		for est in estimators:
			print(est[0])
			fitr = est[1](train, features, response)
			rate, time = do_test_prediction(fitr, test, features, response, args.single)
			print('Prediction rate', rate)
			print('Prediction time [s]', time)
			est[2] += rate
			est[3] += time

	for est in estimators:
		print(est[0], sum(est[2])/len(est[2])*100, min(est[2]), max(est[2]), 'time avg =', median(est[3]))

	if args.dump is not None:
		if args.dump == 'best':
			newest = list(estimators)
			newest.sort(lambda avg1, avg2: cmp(avg1, avg2), lambda est: sum(est[2])/len(est[2]), True)
			selected = newest[0]
		else:
			selected = [est for est in estimators if est[0] == args.dump][0]
		fitr = selected[1](base, features, response)
		from joblib import dump
		k = {'model_name': selected[0], 'features': features, 'fitr': fitr}
		dump(k, 'saved_model.bin')
		print('saved model', selected[0])


if __name__=='__main__':
  main()