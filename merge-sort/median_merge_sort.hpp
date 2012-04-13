#include "include.hpp"

#define BASE_CASE 256
extern off_t threshold;

template <typename T>
struct median_t {
    T* array;
    size_t index;
    int segment;
    size_t rank;

    median_t(T *a = NULL, size_t i = 0, int seg = 0, size_t r = 0)
        : array(a), index(i), segment(seg), rank(r)
    { }

    T&
    operator()() {
        return array[index];
    }
};

template <typename T>
median_t<T>
median(T *a1, T *a2, size_t i1, size_t j1, size_t i2, size_t j2) {
    // printf("median({%d, %d}, {%d, %d}\n", i1, j1, i2, j2);

    size_t size = (j1 - i1) + (j2 - i2);
    assert(size > 1);
    size_t middle = (size % 2) ? size/2 : size/2-1;
    //printf("size: %d, middle: %d\n", size, middle);
    assert(middle > 0);

    if (i1 == j1) {
        return median_t<T>(a2, i2 + middle, 1, middle);
    }
    if (i2 == j2) {
        return median_t<T>(a1, i1 + middle, 0, middle);
    }

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
                    return median_t<T>(a1, m, 0, m + 0 - i1);
                }
            }

            // Median is on left side.
            j = m;
            continue;
        }

        // printf("m = %u, k = %u\n", m, k);
        if (k >= j2) {
            // Out of bounds. Median is in right side.
            i = m + 1;
            continue;
        }

        if (k + 1 < j2) {
            if (a1[m] >= a2[k] && a1[m] <= a2[k+1]) {
                // Found the median.
                return median_t<T>(a1, m, 0, m - i1 + (k+1 - i2));
            }
            if (a1[m] >= a2[k]) {
                // a1[m] is far too big.
                j = m;
                continue;
            }
            // a1[m] is far too small.
            i = m + 1;
        } else {
            assert(k + 1 == j2);
            if (a1[m] >= a2[k]) {
                // Found the median.
                return median_t<T>(a1, m, 0, m - i1 + (j2 - i2));
            }
            // a1[m] is far too small.
            i = m + 1;
        }

    } // while ()

    median_t<T> ret = median(a2, a1, i2, j2, i1, j1);
    ret.segment = 1;
    return ret;
}

template<typename T>
void
parallel_merge(T *a1, T *a2, T* out, size_t outi, size_t i1, size_t j1, size_t i2, size_t j2) {
    size_t s1 = j1-i1, s2 = j2-i2;
    // printf("parallel_merge(%p, %p, {%d}, {%d, %d}, {%d, %d}, {%d, %d})\n", a1, a2, outi, i1, j1, i2, j2, s1, s2);

    /*
    printf("Merging: { ");
    for (int z = i1; z < j1; ++z) {
        printf("%2d, ", a1[z]);
    }
    printf(" } and { ");
    for (int z = i2; z < j2; ++z) {
        printf("%2d, ", a2[z]);
    }
    printf(" }\n");
    */

    if (s1 + s2 <= BASE_CASE) {
        // Perform serial merge.
        std::merge(a1 + i1, a1 + j1, a2 + i2, a2 + j2, out+outi);
        // printf("early return\n");
        return;
    }

    median_t<T> m = median(a1, a2, i1, j1, i2, j2);
    // printf("After call to median, segment: %d\n", m.segment);

    if (m.segment == 1) {
        // printf("swapping a1 & a2\n");
        std::swap(a1, a2);
        std::swap(i1, i2);
        std::swap(j1, j2);
        std::swap(s1, s2);
    }

    size_t a = m.index + 1;
    size_t b = i2 + ((m.rank) - (a - i1));
    b = std::min((size_t)j2, b+1);
    // assert(b < 100000);
    // printf("m.index: %d, m.rank: %d, a: %d, b: %d, i2: %d, j2: %d\n", m.index, m.rank, a, b, i2, j2);
    assert(b <= j2);
    assert(a-i1 + b-i2 == m.rank + 1);
    //printf("a1[%d] = %d, a2[%d] = %d\n", a, a1[a], b, a2[b]);
    assert(i2 <= b);
    cilk_spawn parallel_merge(a1, a2, out, outi, i1, a, i2, b);

    // printf("outi: %d, Len1: %d, Len2: %d, Index: %d, Rank: %d\n", outi, a-i1, b-i2, outi + (a-i1) + (b-i2) - 1, i1 + i2 + m.rank);
    // assert(out[outi + a-i1 + b-i2 - 1] == m());

    parallel_merge(a1, a2, out, outi + a-i1 + b-i2, a, j1, b, j2);
    cilk_sync;
    assert(out[outi + a-i1 + b-i2 - 1] == m());
}

template<typename T>
void
parallel_copy(T *pin, T *pout, size_t i, size_t j, size_t outi) {
    if ((off_t)(j - i) <= threshold) {
        std::copy(pin + i, pin + j, pout + outi);
        return;
    }

    size_t m = i + (j-i)/2;
    cilk_spawn parallel_copy(pin, pout, i, m, outi);
    parallel_copy(pin, pout, m, j, outi + (j-i)/2);
    cilk_sync;
}

template<typename T>
void
parallel_merge_sort(T *a, T *buff, size_t i, size_t j, size_t buffi) {
    size_t s = j - i;

    if (s <= (size_t)threshold) {
        std::sort(a + i, a + j);
        return;
    }

    size_t m = i + (j-i)/2;
    cilk_spawn parallel_merge_sort(a, buff, i, m, buffi);
    parallel_merge_sort(a, buff, m, j, buffi + (j-i)/2);
    cilk_sync;

    parallel_merge(a, a, buff, buffi, i, m, m, j);
    parallel_copy(buff, a, buffi, buffi + (j-i), i);
}
