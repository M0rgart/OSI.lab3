#include "pool.h"
#include "timsort.h"
#include "common.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

typedef enum { TASK_SORT, TASK_MERGE, TASK_STOP } task_kind_t;

typedef struct task {
    task_kind_t kind;
    int *arr;
    int lo, mid, hi;
    struct task *next;
} task_t;

static task_t *q_head = NULL, *q_tail = NULL;
static CRITICAL_SECTION q_cs;
static CONDITION_VARIABLE q_cv;
static CONDITION_VARIABLE q_done_cv;
static int q_stop = 0;
static int q_active = 0;

static void q_init(void) {
    InitializeCriticalSection(&q_cs);
    InitializeConditionVariable(&q_cv);
    InitializeConditionVariable(&q_done_cv);
    q_head = q_tail = NULL;
    q_stop = 0;
    q_active = 0;
}

static void q_destroy(void) {
    DeleteCriticalSection(&q_cs);
}

static void q_push(task_t *t) {
    EnterCriticalSection(&q_cs);
    t->next = NULL;
    if (q_tail) q_tail->next = t;
    else        q_head = t;
    q_tail = t;
    q_active++;
    WakeConditionVariable(&q_cv);
    LeaveCriticalSection(&q_cs);
}

static task_t *q_pop(void) {
    EnterCriticalSection(&q_cs);
    while (!q_head && !q_stop) {
        SleepConditionVariableCS(&q_cv, &q_cs, INFINITE);
    }
    task_t *t = q_head;
    if (t) {
        q_head = t->next;
        if (!q_head) q_tail = NULL;
    }
    LeaveCriticalSection(&q_cs);
    return t;
}

static void q_task_done(void) {
    EnterCriticalSection(&q_cs);
    q_active--;
    if (q_active == 0) WakeAllConditionVariable(&q_done_cv);
    LeaveCriticalSection(&q_cs);
}

static void q_wait_all_done(void) {
    EnterCriticalSection(&q_cs);
    while (q_active > 0) {
        SleepConditionVariableCS(&q_done_cv, &q_cs, INFINITE);
    }
    LeaveCriticalSection(&q_cs);
}

static DWORD WINAPI worker_main(LPVOID arg) {
    (void)arg;
    for (;;) {
        task_t *t = q_pop();
        if (!t) break;
        if (t->kind == TASK_SORT) {
            timsort_range(t->arr, t->lo, t->hi);
        } else if (t->kind == TASK_MERGE) {
            int *buf = (int *)malloc((size_t)(t->mid - t->lo) * sizeof(int));
            if (!buf) handle_error("malloc(merge buf)");
            merge_two(t->arr, t->lo, t->mid, t->hi, buf);
            free(buf);
        }
        free(t);
        q_task_done();
    }
    return 0;
}

void pool_timsort(int *a, int n, int threads) {
    if (n < 2) return;
    if (threads < 1) threads = 1;

    q_init();

    HANDLE *hth = (HANDLE *)calloc((size_t)threads, sizeof(HANDLE));
    if (!hth) handle_error("calloc(handles)");

    for (int i = 0; i < threads; i++) {
        hth[i] = CreateThread(NULL, 0, worker_main, NULL, 0, NULL);
        if (!hth[i]) handle_error("CreateThread");
    }

    int chunk = (n + threads - 1) / threads;
    int nblocks = 0;
    int *ends = (int *)malloc((size_t)threads * sizeof(int));
    if (!ends) handle_error("malloc(ends)");

    for (int i = 0; i < threads; i++) {
        int lo = i * chunk;
        int hi = (lo + chunk > n) ? n : lo + chunk;
        if (lo >= hi) continue;
        task_t *t = (task_t *)calloc(1, sizeof(task_t));
        if (!t) handle_error("calloc(task)");
        t->kind = TASK_SORT;
        t->arr  = a;
        t->lo   = lo;
        t->hi   = hi;
        q_push(t);
        ends[nblocks++] = hi;
    }

    q_wait_all_done();

    while (nblocks > 1) {
        int next = 0;
        int cur_lo = 0;
        for (int i = 0; i < nblocks; i += 2) {
            if (i + 1 >= nblocks) {
                ends[next++] = ends[i];
                continue;
            }
            int mid = ends[i];
            int hi  = ends[i + 1];
            task_t *t = (task_t *)calloc(1, sizeof(task_t));
            if (!t) handle_error("calloc(merge)");
            t->kind = TASK_MERGE;
            t->arr  = a;
            t->lo   = cur_lo;
            t->mid  = mid;
            t->hi   = hi;
            q_push(t);
            ends[next++] = hi;
            cur_lo = hi;
        }
        nblocks = next;
        q_wait_all_done();
    }
    free(ends);

    EnterCriticalSection(&q_cs);
    q_stop = 1;
    WakeAllConditionVariable(&q_cv);
    LeaveCriticalSection(&q_cs);
    WaitForMultipleObjects((DWORD)threads, hth, TRUE, INFINITE);
    for (int i = 0; i < threads; i++) CloseHandle(hth[i]);
    free(hth);

    q_destroy();
}