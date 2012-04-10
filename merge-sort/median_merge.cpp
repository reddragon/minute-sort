#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <assert.h>

using namespace std;

#define spawn 

template <typename T>
struct median_t {
    T* array;
    size_t index;
    size_t rank;

    median_t(T *a = NULL, size_t i = 0, size_t r = 0)
        : array(a), index(i), rank(r)
    { }

    T&
    operator()() {
        return array[index];
    }
};

template <typename T>
median_t<T>
median(T *a1, T *a2, size_t i1, size_t j1, size_t i2, size_t j2) {
    printf("median(%p, %p, %d, %d, %d, %d\n", a1, a2, i1, j1, i2, j2);
    if (i1 == j1) {
        return median_t<T>(a1, i2 + (j2-i2)/2, (j2-i2)/2);
    }
    if (i2 == j2) {
        return median_t<T>(a1, i1 + (j1-i1)/2, (j1-i1)/2);
    }
    size_t size = (j1 - i1) + (j2 - i2);
    assert(size > 1);
    size_t middle = size/2;
    assert(middle > 0);

    // Assume that median is in a1
    size_t i = i1, j = j1, m;
    while (i != j) {
        m = i + (j-i)/2;
        // printf("check if %d (index %d) is the median\n", a1[m], m);

        // Is a1[m] the median?
        size_t k = i2 + (middle - (m - i1 + 1));
        // printf("i2: %d, middle: %d, m: %d, i1: %d\n", i2, middle, m, i1);

        if (m-i1+1 > middle) {
            // k is the first element in a2.
            if (m-i1+1-middle == 1) {
                if (a1[m] <= a2[i2]) {
                    // Found median.
                    return median_t<T>(a1, m, middle);
                }
            }

            // Median is on left side.
            j = m;
            continue;
        }

        // printf("m = %u, k = %u\n", m, k);
        if (k > j2) {
            // Out of bounds. Median is in right side.
            i = m + 1;
            continue;
        }

        if (k + 1 < j2) {
            if (a1[m] >= a2[k] && a1[m] <= a2[k+1]) {
                // Found the median.
                return median_t<T>(a1, m, middle);
            }
            if (a1[m] >= a2[k]) {
                // a1[m] is far too big.
                j = m;
                continue;
            }
            // a1[m] is far too small.
            i = m + 1;
        } else {
            if (a1[m] >= a2[k]) {
                // Found the median.
                return median_t<T>(a1, m, middle);
            }
            // a1[m] is far too small.
            i = m + 1;
        }

    } // while ()

    return median(a2, a1, i2, j2, i1, j1);
}

template<typename T>
void
parallel_merge(T *a1, T *a2, T* out, int outi, int i1, int j1, int i2, int j2) {
    size_t s1 = j1-i1, s2 = j2-i2;
    printf("parallel_merge(%p, %p, {%d}, {%d, %d}, {%d, %d}, {%d, %d})\n", a1, a2, outi, i1, j1, i2, j2, s1, s2);
    printf("Merging: { ");
    for (int z = i1; z < j1; ++z) {
        printf("%2d, ", a1[z]);
    }
    printf(" } and { ");
    for (int z = i2; z < j2; ++z) {
        printf("%2d, ", a2[z]);
    }
    printf(" }\n");

    if (s1 + s2 <= 2 || i1 == j1 || i2 == j2) {
        // Perform serial merge.
        std::merge(a1 + i1, a1 + j1, a2 + i2, a2 + j2, out+outi);
        return;
    }

    median_t<int> m = median(a1, a2, i1, j1, i2, j2);

    if (m.array == a2) {
        std::swap(a1, a2);
        std::swap(i1, i2);
        std::swap(j1, j2);
    }

    size_t i = m.index + 1;
    size_t j = m.rank - (m.index - i1) + i2;
    assert(j < 100000);
    assert(i2 <= j);
    printf("m.index: %d, m.rank: %d, j: %d, j2: %d\n", m.index, m.rank, j, j2);
    assert(j <= j2);

    printf("a1[%d] = %d, a2[%d] = %d\n", i, a1[i], j, a2[j]);
    parallel_merge(a1, a2, out, outi, i1, i, i2, j);
    parallel_merge(a1, a2, out, outi + (i-i1) + (j-i2), i, j1, j, j2);
}

void
test(int *a1, int *a2, int i1, int j1, int i2, int j2) {
    int s1 = j1-i1, s2=j2-i2;
    vector<int> out(s1 + s2);

    vector<int> v(a1 + i1, a1 + j1);
    v.insert(v.end(), a2 + i2, a2 + j2);
    sort(v.begin(), v.end());

    median_t<int> m = median(a1, a2, i1, j1, i2, j2);

#if 0
    printf("Median: %s[%d] = %d (rank: %d)\n", (m.array == a1 ? "a1" : "a2"), m.index, m(), m.rank);
    printf("Median by sorting: %d, #elements = %d (%d)\n", v[v.size()/2], v.size(), s1+s2);

    if (m.array == a2) {
        std::swap(a1, a2);
        std::swap(i1, i2);
        std::swap(j1, j2);
    }

    size_t i = m.index + 1;
    size_t j = m.rank - (m.index - i1) + i2;

    printf("a1[%d] = %d, a2[%d] = %d\n", i, a1[i], j, a2[j]);
    std::merge(a1 + i1, a1+i,  a2 + i2, a2 + j, out.begin());
    std::merge(a1 + i,  a1+j1, a2 + j,  a2 + j2, out.begin() + m.rank + 1);
#else
    parallel_merge(a1, a2, &(*out.begin()), 0, i1, j1, i2, j2);
#endif

    for (size_t k = 0; k < out.size(); ++k) {
        printf("%2d ", out[k]);
    }
    printf("\n\n");

    assert(out[(s1+s2)/2] == m());
    for (int i = 0; i < (int)out.size(); i++)
        assert(out[i] == v[i]);
}

int
main() {
    int a1[] = { 23, 45, 55, 56, 57, 58, 59, 65, 75, 85 };
    int a2[] = { 10, 20, 30, 40, 50, 60, 70 };
    int s1 = sizeof(a1)/sizeof(int), s2 = sizeof(a2)/sizeof(int);

    printf("A1::%p, A2::%p\n", a1, a2);
    test(a1, a2, 0, s1, 0, s2);
    test(a2, a1, 0, s2, 0, s1);
    test(a1, a2, 1, s1, 1, s2);
    test(a2, a1, 1, s2, 1, s1);
    test(a1, a2, 2, s1, 2, s2);
    test(a2, a1, 2, s2, 2, s1);
    test(a1, a2, 3, s1, 3, s2);
    test(a2, a1, 3, s2, 3, s1);

}
