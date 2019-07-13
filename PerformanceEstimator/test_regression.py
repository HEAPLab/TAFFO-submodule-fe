#!/usr/bin/env python2
__doc__ = '''Use RandomForestClassifier to classify whether Fix is faster'''
import pandas as pd
import numpy as np
from math import sqrt
from stats_loader import load_stats
from profile_loader import load_profile
import time as pytime
import argparse
from preprocess import *

better_than = 0.2  # 1=go fixed if time is less than float, 0.5 if time is half then float, etc.


def load_data(path, features=None, response=None, boostfail=0):
    new_features, d3 = load_data_phase1(path)

    d3['ratio'] = d3['flo_T'] / d3['fix_T']

    if boostfail > 0:
        for rowi in d3.index:
            if d3.loc[rowi, 'ratio'] < 1:
                row = d3.loc[rowi, :]
                for repi in range(0, boostfail):
                    cprow = row.copy()
                    cprow.name = (cprow.name[0] + '_c' + str(repi), cprow.name[1])
                    d3 = d3.append(cprow)

    print d3['ratio']

    if not response:
        response = ['ratio']
    if not features:
        features = new_features

    print features
    for f in features:
        if f not in d3.columns.values:
            d3[f] = 0.

    print d3.loc[:, features].shape
    print d3.loc[:, response].shape
    return d3.fillna(0), features, response


def split_train_test(d3):
    msk = [n < 0.8 * len(d3) for n in range(0, len(d3))]
    np.random.shuffle(msk)
    train = d3[msk]
    test = d3[[not v for v in msk]]
    return train, test


def do_prediction(fitr, test, features, response, oneatatime):
    return do_prediction_2(fitr, test.loc[:, features], test.loc[:, response], oneatatime)


def do_prediction_2(fitr, test_feat, test_resp, oneatatime):
    if not oneatatime:
        benchs = [test_feat.index]
    else:
        benchs = map(lambda x: [x], test_feat.index)
    results, times = [], []
    for bench in benchs:
        res = fitr.score(test_feat.loc[bench], np.ravel(test_resp.loc[bench]))
        t0 = pytime.time()
        pred = fitr.predict(test_feat.loc[bench])
        t1 = pytime.time()
        print 'Predict', bench, pred
        print 'Actual', np.ravel(test_resp.loc[bench])
        results += [res]
        times += [t1 - t0]
    return results, times


def linear(train, test, features, response, oneatatime):
    from sklearn import linear_model
    regr = linear_model.LinearRegression()
    fitr = regr.fit(train.loc[:, features], np.ravel(train.loc[:, response]))
    return do_prediction(fitr, test, features, response, oneatatime)


def linearpoly(train, test, features, response, oneatatime):
    from sklearn.linear_model import LinearRegression
    from sklearn.preprocessing import PolynomialFeatures
    from sklearn.pipeline import Pipeline
    regr = Pipeline([('poly', PolynomialFeatures(degree=2)),
                     ('linear', LinearRegression())])
    fitr = regr.fit(train.loc[:, features], np.ravel(train.loc[:, response]))
    return do_prediction(fitr, test, features, response, oneatatime)


def nn(train, test, features, response, oneatatime):
    from sklearn.neural_network import MLPRegressor
    regr = MLPRegressor(hidden_layer_sizes=(5000,25), solver='lbfgs')
    fitr = regr.fit(train.loc[:, features], np.ravel(train.loc[:, response]))
    return do_prediction(fitr, test, features, response, oneatatime)


if __name__ == '__main__':
    from sys import argv

    parser = argparse.ArgumentParser()
    parser.add_argument('pred', help='input set', nargs='?', default='./data/20190623_polybench_antarex/')
    parser.add_argument('--train', '-t', help='training set', default='./data/20190623_polybench_antarex/')
    parser.add_argument('--test', '-T', action='store_true',
                        help='run in test mode (use as training set random partition of training set)')
    parser.add_argument('--ntry', '-n', type=int, help='number of retries', default=100)
    parser.add_argument('--single', '-s', action='store_true', help='only predict one bench at a time')
    parser.add_argument('--boostfail', '-f', type=int,
                        help='amplify inputs in training set with low speedup by the given factor', default=5)
    args = parser.parse_args()

    # base_path='./20180911_multiconf_results/'
    base_path = args.train
    path = args.pred
    base, features, response = load_data(base_path, boostfail=args.boostfail)
    train = base
    print path

    # import warnings
    # warnings.simplefilter("error")
    test, f_2, r_2 = load_data(path, features, response)
    # print test

    estimators = [
        ['Linear Regression', linear, [], []],
       # ['Polynomial Linear Regression', linearpoly, [], []],
        ['Neural Network', nn, [], []],
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
        print est[0], sum(est[2]) / len(est[2]) * 100, min(est[2]), max(est[2]), 'time avg =', median(est[3])

