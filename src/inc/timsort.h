#ifndef TIMSORT_H
#define TIMSORT_H

#include <stddef.h>

#define MIN_RUN 32

void timsort_serial(int *a, size_t n);
void timsort_range(int *a, int lo, int hi);
void merge_two(int *a, int lo, int mid, int hi, int *buf);

#endif