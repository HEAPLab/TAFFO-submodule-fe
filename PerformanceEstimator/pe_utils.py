#!/usr/bin/env python2


def median(iter):
	iter.sort()
	if len(iter) % 2 == 1:
		return iter[len(iter)/2]
	else:
		return (iter[len(iter)/2] + iter[len(iter)/2+1]) / 2
