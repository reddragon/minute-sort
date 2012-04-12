#include <vector>
#include <iostream>
#include <algorithm>
#include <stack>
#include <stdio.h>

using namespace std;

// 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8

// 1 2 3 4 5 6 7 8 9 10 11
// 0 1 0 2 0 1 0 3 0  2  0
// 1 2 1 3 1 2 1 4 1  3  1

inline bool
is_pow2(int n) {
    return (n & -n) == n;
}

int log2(int n) {
    int lg2 = 0;
    while (n > 1) {
        n /= 2;
        ++lg2;
    }
    return lg2;
}

template <typename T>
void
merge_sort_bottom_up(T *a, int i, int j) {
    int sz = j - i;
    // printf("merge_sort_bottom_up(%d, %d, %d)\n", i, j, sz);

    if (sz < 2) {
        return;
    }

    int r = 2;
    int len_max = 2;
    vector<T> tmp(sz);
    T *buff = &(tmp.front());

    while (r < j) {
        int mpow2 = (r & -r);
        len_max = mpow2 > len_max ? mpow2 : len_max;
        int t = 2;
        // printf("t: %d, mpow2: %d\n", t, mpow2);
        while (t <= mpow2) {
            int csz = t / 2;
            T *c1 = a + r - t;
            T *c2 = a + r - csz;
            // printf("merging 2 segments of length %d from index %d & %d\n", csz, r-t, r-csz);
            std::merge(c1, c1 + csz, c2, c2 + csz, buff);
            std::copy(buff, buff + t, c1);
            t *= 2;
        }
        r += 2;
    }

    // printf("r: %d\n", r);
    if ((sz & -sz) != sz) {
        // input is not a power of 2. perform a final merge pass
        r = 1 << (log2(r) + 1);
        // printf("r is now: %d\n", r);

        len_max *= 2;
        int t = 2;
        while (t <= len_max) {
            int csz = t / 2;
            T *c1 = a + r - t;
            T *c2 = a + r - csz;
            if (c2 < a + j) {
                int sz2 = j - (r - csz);
                // printf("merging 2 segments of length %d from index %d & %d\n", csz, r-t, r-csz);
                std::merge(c1, c1 + csz, c2, c2 + sz2, buff);
                std::copy(buff, buff + csz + sz2, c1);
            }
            t *= 2;
        }
    }
}

int
main() {
    /*
    int i = 1;
    int n = 18;
    while (i < n) {
        printf("%3d, ", log2(i & -i) + 1);
        ++i;
    }
    */

    /*
    int a[ ] = { 34, 23, 65, 11, 3, 21, 9, 71, 9, 12, 32, 233, 43 };
    int sz = sizeof(a)/sizeof(int);
    merge_sort_bottom_up(a, 0, sz);
    for (int i = 0; i < sz; ++i) {
        printf("%3d, ", a[i]);
    }
    printf("\n");
    */
    vector<int> v(10000000);
    // merge_sort_bottom_up(&v.front(), 0, v.size());
    std::sort(v.begin(), v.end());
}
