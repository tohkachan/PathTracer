#ifndef QUICKSELECT_H
#define QUICKSELECT_H

#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <assert.h>

/*
 *
 * An implementation of an randomized in-place quickselect
 * (http://en.wikipedia.org/wiki/Quickselect)
 * The result of this algorithm is that the data between start (incl)
 * and end (excl) is rearranged so that all elements before wanted
 * are less than wanted, and all elements after wanted are greater
 * than wanted
 * class It - the type of the iterator
 * class Cmp - the type of the comparator, which when called on two values,
 *  return true iff the first is strictly less than the second
 * depth - the recursion depth, leave as zero
 * start - the beginning of the modified area
 * end   - the end of the modified area
 * wanted - the pivot of the algorithm
 * complexity - linearithmicish? expected linear? slow?
*/
template<class It, class T>
void quick_select(std::minstd_rand &generator, int depth, It start, It end, It wanted, std::function<bool(T lhs, T rhs)> cmp) {
	if (start == end) {
		return;
	}
	assert(start < end);
	assert(wanted >= start);
	assert(wanted < end);
	assert(depth < 500);
	int dis = (int)std::distance(start, end);
	int index = dis / 2;
	if (dis > 10) {
		std::uniform_int_distribution<int> dist(0, dis - 1);
		index = dist(generator);
		assert(index < dis);
	}
	typename It::value_type pivot = *(start + index);
	It left = start;
	It right = end - 1;
	while (left < right) {
		while (cmp(*left, pivot)) {
			assert(left < end);
			left++;
		}
		while (cmp(pivot, *right)) {
			assert(right > start);
			right--;
		}
		if (left == right) {
			break;
		}
		typename It::value_type a = *left;
		typename It::value_type b = *left;
		if (cmp(*right, *left)) {
			std::swap(*left, *right);
		}
		else {
			assert(!cmp(a, b));
			assert(!cmp(*left, *right));
			if ((generator() & 1) == 1) {
				left++;
			}
			else {
				right--;
			}
		}
	}
	assert(left == right);
	if (wanted < left) {
		assert(left <= end);
		quick_select(generator, depth + 1, start, left, wanted, cmp);
	}
	else if (wanted == left) {
		return;
	}
	else {
		assert(left >= start);
		quick_select(generator, depth + 1, left, end, wanted, cmp);
	}
}

/*
 * Given a range between start (incl) and end (excl),
 * return an iterator in the middle of the range.
*/
template<class It>
It quick_middle(It start, It end) {
	return start + (std::distance(start, end) / 2);
}

/*
 *
 * An implementation of an randomized in-place quickselect
 * (http://en.wikipedia.org/wiki/Quickselect)
 * The result of this algorithm is that the data between start (incl)
 * and end (excl) is rearranged so that the median is at the center,
 * all smaller elements are before the median, and larger elements are
 * after the median.
 *
 * class It - the type of the iterator
 * class Cmp - the type of the comparator, which when called on two values,
 *  return trues iff the first is less than the second
 * depth - the recursion depth, leave as zero
 * start - the beginning of the modified area
 * end   - the end of the modified area
 * returns an iterator to the median
 * complexity - linearithmicish?
*/
template<class It, class T>
It quick_median(It start, It end, std::function<bool(T lhs, T rhs)> cmp) {
	It med = quick_middle(start, end);
	std::minstd_rand generator;
	quick_select(generator, 0, start, end, med, cmp);
	return med;
}

#endif