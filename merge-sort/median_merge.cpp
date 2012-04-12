#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <stdint.h>

using namespace std;

typedef uint64_t inttype;

#if !defined __cilkplusplus
#define cilk_spawn
#define cilk_sync
#define MAIN main
#else
#include <cilkview.h>
#define MAIN cilk_main
#endif

#define BASE_CASE 2
// 4096

template <typename T>
struct median_t {
    T* array;
    int segment;
    size_t index;
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

    if (s1 + s2 <= BASE_CASE /*|| i1 == j1 || i2 == j2*/) {
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
    
    /*
    for (size_t k = 0; k < out.size(); ++k) {
        printf("%2d ", out[k]);
    }
    printf("\n\n");
    */

    for (int i = 0; i < (int)out.size(); i++) {
        assert(out[i] == v[i]);
    }

    assert(out[(s1+s2)/2] == m());
    assert(m.rank == (s1+s2)/2);
    printf("s1: %d, s2: %d, (s1+s2)/2: %d\n", s1, s2, (s1+s2)/2);
    printf("Median: %d, 1 before median: %d, Median by index: %d\n", m(), out[(s1+s2)/2 -1], out[(s1+s2)/2]);
}

template<typename T>
void
parallel_copy(T *pin, T *pout, size_t i, size_t j, size_t outi) {
    if (j - i <= BASE_CASE) {
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

    if (s <= BASE_CASE) {
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

template <typename Iter>
bool
is_sorted(Iter f, Iter l) {
    Iter next = f;
    int i = 0;
    while (f != l) {
        ++next;
        if (next != l && *f > *next) {
            printf("Mismatch at index %d<->%d (%llu > %llu)\n", i, i+1, *f, *next);
            int k = i < 7 ? -i : -7;
            printf("Before: { ");
            while (k != 0) {
                printf("%llu,  ", f[k++]);
            }
            printf("}, After: { ");
            k = 0;
            while (k != 7 && f+k != l) {
                printf("%llu,  ", f[k++]);
            }
            printf(" }\n");
            return false;
        }
        ++i;
        f = next;
    }
    return true;
}

int
MAIN(int argc, char *argv[]) {

    if (argc > 1) {
        freopen(argv[1], "r", stdin);
    }

    /*
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
    */

    vector<inttype> *v1 = new vector<inttype>, *v2 = new vector<inttype>;
    int s1, s2;
    // std::cin >> s1 >> s2;
    scanf("%d %d", &s1, &s2);
    v1->resize(s1);
    v2->resize(s2);
    for (int i = 0; i < s1; i++) {
        scanf("%llu", &((*v1)[i]));
        // std::cin >> v1[i];
    }

    for (int i = 0; i < s2; i++) {
        scanf("%llu", &((*v2)[i]));
        // std::cin >> v2[i];
    }

    inttype* a1 = &v1->front(), *a2 = &v2->front();
#if 0
    test(a1, a2, 0, s1, 0, s2);
#else
    vector<inttype> *out = new vector<inttype>(s1 + s2);
    // parallel_merge(a1, a2, &(*out->begin()), 0, 0, s1, 0, s2);
    // assert(is_sorted(out->begin(), out->end()));
    parallel_merge_sort(a1, &(*out->begin()), 0, s1, 0);
#endif
}
