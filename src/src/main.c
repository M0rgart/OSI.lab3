#include "common.h"
#include "timsort.h"
#include "pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fill_random(int *a, int n, unsigned seed) {
    srand(seed);
    for (int i = 0; i < n; i++) a[i] = rand();
}

static int is_sorted(const int *a, int n) {
    for (int i = 1; i < n; i++) if (a[i] < a[i - 1]) return 0;
    return 1;
}

static void bench(int n, int threads, int reps) {
    int *a = (int *)malloc((size_t)n * sizeof(int));
    if (!a) handle_error("malloc(a)");

    double t_ser = 0, t_par = 0;
    for (int r = 0; r < reps; r++) {
        fill_random(a, n, 1234u + r);
        double t0 = now_sec();
        timsort_serial(a, (size_t)n);
        t_ser += now_sec() - t0;

        fill_random(a, n, 1234u + r);
        t0 = now_sec();
        if (threads == 1) timsort_serial(a, (size_t)n);
        else              pool_timsort(a, n, threads);
        t_par += now_sec() - t0;

        if (!is_sorted(a, n)) {
            fprintf(stderr, "NOT SORTED (n=%d, t=%d)\n", n, threads);
            ExitProcess(2);
        }
    }
    t_ser /= reps;
    t_par /= reps;
    printf("n=%9d t=%2d | serial=%.4fs  parallel=%.4fs  S=%.2f  E=%.2f\n",
           n, threads, t_ser, t_par, t_ser / t_par,
           (t_ser / t_par) / threads);
    free(a);
}

int main(int argc, char **argv) {
    int threads = 1, n = 1000000, reps = 3, bench_mode = 0, verbose = 0;
    const char *in_path = NULL, *out_path = NULL;

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-t") && i + 1 < argc) threads    = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) n          = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-r") && i + 1 < argc) reps       = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-b"))                 bench_mode = 1;
        else if (!strcmp(argv[i], "-v"))                 verbose    = 1;
        else if (!strcmp(argv[i], "-i") && i + 1 < argc) in_path    = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) out_path   = argv[++i];
        else {
            fprintf(stderr,
                "Usage:\n"
                "  %s -t <threads> -n <size> [-i in] [-o out] [-v]   # sort\n"
                "  %s -b -n <size> -t <threads> [-r reps] [-v]       # benchmark\n",
                argv[0], argv[0]);
            return 1;
        }
    }
    if (threads < 1) threads = 1;
    if (reps < 1) reps = 1;

    if (bench_mode) { bench(n, threads, reps); return 0; }

    int *a = (int *)malloc((size_t)n * sizeof(int));
    if (!a) handle_error("malloc(a)");

    if (in_path) {
        FILE *f = fopen(in_path, "rb");
        if (!f) handle_error("fopen(in)");
        size_t got = fread(a, sizeof(int), (size_t)n, f);
        fclose(f);
        n = (int)got;
    } else {
        fill_random(a, n, 42u);
    }

    if (verbose) {
        printf("sorting %d ints with %d thread(s), pid=%lu\n",
               n, threads, (unsigned long)GetCurrentProcessId());
        fflush(stdout);
        Sleep(3000);
    }

    double t0 = now_sec();
    if (threads == 1) timsort_serial(a, (size_t)n);
    else              pool_timsort(a, n, threads);
    double dt = now_sec() - t0;

    if (!is_sorted(a, n)) {
        fprintf(stderr, "ERROR: array is not sorted\n");
        return 2;
    }
    printf("OK: sorted %d ints in %.4f s with %d thread(s)\n",
           n, dt, threads);

    if (out_path) {
        FILE *f = fopen(out_path, "wb");
        if (!f) handle_error("fopen(out)");
        fwrite(a, sizeof(int), (size_t)n, f);
        fclose(f);
    }
    free(a);
    return 0;
}