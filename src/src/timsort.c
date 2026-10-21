#include "timsort.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

static void binary_insertion_sort(int *a, int lo, int hi) {
    for (int i = lo + 1; i < hi; i++) {
        int key = a[i];
        int l = lo, r = i;
        while (l < r) {
            int m = l + (r - l) / 2;
            if (a[m] <= key) l = m + 1;
            else r = m;
        }
        memmove(&a[l + 1], &a[l], (size_t)(i - l) * sizeof(int));
        a[l] = key;
    }
}

static int count_run(int *a, int lo, int hi) {
    if (lo + 1 >= hi) return 1;
    int i = lo + 1;
    if (a[i] < a[lo]) {
        while (i < hi && a[i] < a[i - 1]) i++;
        for (int x = lo, y = i - 1; x < y; x++, y--) {
            int t = a[x]; a[x] = a[y]; a[y] = t;
        }
    } else {
        while (i < hi && a[i] >= a[i - 1]) {
            i++;
            if (i - lo > 1024) break;
        }
    }
    return i - lo;
}

void merge_two(int *a, int lo, int mid, int hi, int *buf) {
    int n1 = mid - lo;
    if (n1 <= 0 || hi <= mid) return;
    memcpy(buf, &a[lo], (size_t)n1 * sizeof(int));
    int i = 0, j = mid, k = lo;
    while (i < n1 && j < hi) {
        if (buf[i] <= a[j]) a[k++] = buf[i++];
        else                a[k++] = a[j++];
    }
    while (i < n1) a[k++] = buf[i++];
}


void timsort_range(int *a, int lo, int hi) {
    int n = hi - lo;
    if (n < 2) return;

    int cap = n / MIN_RUN + 4;
    int *starts = (int *)malloc((size_t)cap * sizeof(int));
    int *lens   = (int *)malloc((size_t)cap * sizeof(int));
    if (!starts || !lens) handle_error("malloc(timsort runs)");
    int cnt = 0;

    int i = lo;
    while (i < hi) {
        int rl = count_run(a, i, hi);
        if (rl < MIN_RUN) {
            int force = (i + MIN_RUN <= hi) ? MIN_RUN : (hi - i);
            binary_insertion_sort(a, i, i + force);
            rl = force;
        }
        starts[cnt] = i;
        lens[cnt] = rl;
        cnt++;
        i += rl;
    }

    int *buf = (int *)malloc((size_t)n * sizeof(int));
    if (!buf) handle_error("malloc(timsort buf)");
    
    int step = 1;
    while (step < cnt) {
        for (int k = 0; k < cnt - step; k += 2 * step) {
            int cur_lo = starts[k];
            int cur_mid = starts[k] + lens[k];
            int next_end = (k + 2 * step < cnt) ? starts[k + 2 * step] : hi;
            
            merge_two(a, cur_lo, cur_mid, next_end, buf);
            
            lens[k] = next_end - cur_lo; 
        }
        step *= 2;
    }
    
    free(buf);
    free(starts);
    free(lens);
}

void timsort_serial(int *a, size_t n) {
    if (n < 2) return;
    timsort_range(a, 0, (int)n);
}