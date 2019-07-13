#!/usr/bin/env python2
__doc__ = '''Use RandomForestClassifier to classify whether Fix is faster'''
import pandas as pd
import numpy as np
from math import sqrt
from stats_loader import load_stats
from profile_loader import load_profile
import time as pytime
import argparse

better_than=0.2 #1=go fixed if time is less than float, 0.5 if time is half then float, etc.


def median(iter):
	iter.sort()
	if len(iter) % 2 == 1:
		return iter[len(iter)/2]
	else:
		return (iter[len(iter)/2] + iter[len(iter)/2+1]) / 2

def preprocess_features_instfrequency(orig_feat, d3):
	new_features = []
	for f in orig_feat:
		a, b = f
		if a[0] != 'B':
			if b == 'fix':
				f1 = (a, 'float')
				d3[a] = (d3[f] - d3[f1]) / d3[('*', 'float')].astype(float)
				if d3[a].nunique() > 1:
					new_features.append(a)
		else:
			parts = a.split('_')
			if parts[1] != 'n':
				new_features.append(a)
			if b == 'fix':
				f1 = (a, 'float')
				n = d3[(parts[0] + '_n_*', 'float')].fillna(1).replace(0, 1).astype(float)
				d3[a] = (d3[f] - d3[f1]) / n
				if d3[a].nunique() > 1:
					new_features.append(a)
	return new_features


def preprocess_features_noop(orig_feat, d3):
	nblock_threshold = 2
	new_features = []
	for f in orig_feat:
		a, b = f
		parts = a.split('_')
		if len(parts) == 3 and parts[1] == 'contain':
			nblock2 = int(parts[2][1:])
			if nblock2 > nblock_threshold:
				continue
		nblock = int(parts[0][1:])
		if nblock > nblock_threshold:
			continue
		if parts[1] == 'minDist':
			continue
		elif parts[1] != 'n':
			if b == 'fix':
				new_features += [a]
				d3[a] = d3[(a, b)]
		else:
			if not parts[2][0].isupper():
				continue
			if parts[2][0:5] == 'call(':
				continue
			if parts[2] == '*':
				continue
			new_features += [b+'_'+a]
			d3[b+'_'+a] = d3[(a,b)] / d3[(parts[0] + '_n_*', b)].replace(0, 1).astype(float)
	return new_features


def load_data(path,features=None,response=None, boostfail=0):
	d1=load_stats(path)
	d2=load_profile(path)

	d3=pd.concat([d1,d2],axis=1)
	d3=d3.drop('durbin',axis=0,errors='ignore')
	d3=d3.fillna(0)

	new_features = preprocess_features_instfrequency(d1.columns.values, d3)
	#new_features = preprocess_features_noop(d1.columns.values, d3)

	d3['ratio'] = d3['flo_T'] / d3['fix_T']
	w1 = d3['fix_T'] < (d3['flo_T'] * better_than)
	w2 = d3['fix_T'] < (d3['flo_T'])
	# d3['worth']=w1.astype(int)+w2.astype(int)-1
	#d3['worth']=d3['fix_T'] < (d3['flo_T'])
	d3['worth'] = ((d3['ratio'] - 1.0) / 0.2).astype(int).clip(-1, 1)
	# d3['worth'] = w1

	if boostfail > 0:
		for rowi in d3.index:
			if int(d3.loc[rowi,'worth']) < 1:
				row = d3.loc[rowi,:]
				for repi in range(0,boostfail):
					cprow = row.copy()
					cprow.name = (cprow.name[0] + '_c' + str(repi), cprow.name[1])
					d3=d3.append(cprow)

	#print d3['ratio'], d3['worth']
	print '<20%', sum(w1), 'of', len(d3['worth'])
	print '<100%', sum(w2)-sum(w1), 'of', len(d3['worth'])
	print '>100%', len(d3['worth'])-sum(w2), 'of', len(d3['worth'])

	if not response: 
		response=['worth']
	if not features: 
		features=new_features

	print features
	for f in features:
		if f not in d3.columns.values :
			d3[f]=0.
			
	print d3.loc[:,features].shape
	print d3.loc[:,response].shape
	return d3.fillna(0), features, response

def split_train_test(d3):
	msk = [n < 0.8 * len(d3) for n in range(0, len(d3))]
	np.random.shuffle(msk)
	train = d3[msk]
	test  = d3[[not v for v in msk]]
	return train, test


def do_prediction(fitr, predictor, test, features, response, oneatatime):
	if not oneatatime:
		benchs=[test.index]
	else:
		benchs=map(lambda x: [x], test.index)
	results, times = [], []
	for bench in benchs:
		res = fitr.score(test.loc[bench, features], np.ravel(test.loc[bench, response]))
		t0 = pytime.time()
		pred = fitr.predict(test.loc[bench, features])
		t1 = pytime.time()
		print 'Predict', bench, pred
		print 'Actual', np.ravel(test.loc[bench, response])
		results += [res]
		times += [t1 - t0]
	return results, times


def adaboost(train, test, features, response, oneatatime):
	from sklearn.ensemble import AdaBoostClassifier
	regr=AdaBoostClassifier(n_estimators=100)
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def gradient(train, test, features, response, oneatatime):
	from sklearn.ensemble import GradientBoostingClassifier
	regr=GradientBoostingClassifier(max_features=int(sqrt(len(features))))
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def extree(train, test, features, response, oneatatime):
	from sklearn.ensemble import ExtraTreesClassifier
	regr=ExtraTreesClassifier(max_features=int(sqrt(len(features))))
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def bagging(train, test, features, response, oneatatime):
	from sklearn.ensemble import BaggingClassifier
	regr=BaggingClassifier(max_features=int(sqrt(len(features))))
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def randomforest(train, test, features, response, oneatatime):
	from sklearn.ensemble import RandomForestClassifier
	regr=RandomForestClassifier(max_features=int(sqrt(len(features))))
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def multilayerperceptron(train, test, features, response, oneatatime):
	from sklearn.neural_network import MLPClassifier
	regr = MLPClassifier(solver='lbfgs', alpha=1e-5, hidden_layer_sizes=(int(len(features)/2), int(len(features)/8)), random_state=1)
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def svc(train, test, features, response, oneatatime):
	from sklearn.svm import SVC
	regr = SVC()
	fitr=regr.fit(train.loc[:,features],np.ravel(train.loc[:,response]))
	return do_prediction(fitr, regr, test, features, response, oneatatime)


def pipeline(train, test, features, response):
	from sklearn.pipeline import Pipeline, FeatureUnion
	from sklearn.model_selection import GridSearchCV
	from sklearn.svm import SVC
	from sklearn.datasets import load_iris
	from sklearn.decomposition import PCA
	from sklearn.feature_selection import SelectKBest

	X, y = train.loc[:,features], np.ravel(train.loc[:,response])
	pca = PCA(n_components=2)
	selection = SelectKBest(k=1)
	combined_features = FeatureUnion([("pca", pca), ("univ_select", selection)])
	X_features = combined_features.fit(X, y).transform(X)
	svm = SVC(kernel="linear")
	pipeline = Pipeline([("features", combined_features), ("svm", svm)])
	param_grid = dict(features__pca__n_components=[1, 2, 3],
                  features__univ_select__k=[1, 2],
                  svm__C=[0.1, 1, 10])
	grid_search = GridSearchCV(pipeline, param_grid=param_grid, verbose=0, n_jobs=8)
	grid_search.fit(X, y)
	est=grid_search.best_estimator_
	#print "Best Estimator"
	#print est
	print 'Predict', est.predict(test.loc[:,features])
	print 'Actual', np.ravel(test.loc[:,response])
	res= est.score(test.loc[:,features], np.ravel(test.loc[:,response]))
	print res
	return res


if __name__=='__main__' :
	from sys import argv

	parser = argparse.ArgumentParser()
	parser.add_argument('pred', help='input set', nargs='?', default='./data/20190623_polybench_antarex/')
	parser.add_argument('--train', '-t', help='training set', default='./data/20190623_polybench_antarex/')
	parser.add_argument('--test', '-T', action='store_true', help='run in test mode (use as training set random partition of training set)')
	parser.add_argument('--ntry', '-n', type=int, help='number of retries', default=100)
	parser.add_argument('--single', '-s', action='store_true', help='only predict one bench at a time')
	parser.add_argument('--boostfail', '-f', type=int, help='amplify inputs in training set with low speedup by the given factor', default=5)
	args = parser.parse_args()

	#base_path='./20180911_multiconf_results/'
	base_path = args.train
	path = args.pred
	base, features, response = load_data(base_path, boostfail=args.boostfail)
	train=base
	print path
	
	#import warnings
	#warnings.simplefilter("error")
	test, f_2, r_2 = load_data(path, features, response)
	#print test

	estimators = [
		['Random Forest',              randomforest,         [], []],
		['Extremely Randomized Trees', extree,               [], []],
		['Bagging',                    bagging,              [], []],
		['AdaBoost',                   adaboost,             [], []],
		['Gradient Tree',              gradient,             [], []],
		['Multilayer Perceptron',      multilayerperceptron, [], []],
		['SVC',                        svc,                  [], []],
		#['Grid Search Best Estimator', pipeline, [], []] # takes hours and hours
	]
	n = args.ntry
	for i in range(n):
		if args.test:
			train, test = split_train_test(base)
		for est in estimators:
			print est[0]
			rate, time = est[1](train, test, features, response, args.single)
			print 'Prediction rate', rate
			print 'Prediction time [s]', time
			est[2] += rate
			est[3] += time

	for est in estimators:
		print est[0], sum(est[2])/len(est[2])*100, min(est[2]), max(est[2]), 'time avg =', median(est[3])

