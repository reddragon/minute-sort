/* -*- Mode: C++; c-basic-offset: 8; indent-tabs-mode: t -*- */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include <algorithm>

// 
// TODO:
// 
// [1] Fetch the memory & cache parameters
// [2] Try M/B way merge sort
// [3] Implement parallel merge
// 

//#define NRECORDS  1000000
//#define THRESHOLD     100
off_t nrecords, threshold, szrecord;

#if !defined __cilkplusplus
#define cilk_spawn 
#define cilk_sync 
#define cilk_main main
#endif

#define BASE_CASE 256

using namespace std;

char * file_ptr, * scratch;

struct FileRecord {
	mutable char base[100];

	bool
	operator<(FileRecord const &rhs) const {
		// this->base[99] = '\0';
		// rhs.base[99] = '\0';
		bool res = memcmp(this->base, rhs.base, szrecord) < 0;
		// fprintf(stderr, "%s is %s %s\n", this->base, (res ? "<" : ">="), rhs.base);
		return res;
	}

	bool
	operator<=(FileRecord const &rhs) const {
		bool res = memcmp(this->base, rhs.base, szrecord) <= 0;
		return res;
	}

	bool
	operator>=(FileRecord const &rhs) const {
		bool res = memcmp(this->base, rhs.base, szrecord) >= 0;
		return res;
	}

	bool
	operator==(FileRecord const &rhs) const {
		bool res = (memcmp(this->base, rhs.base, szrecord) == 0);
		return res;
	}

};

struct FileIterator {
	char *base;
	size_t offset;

	typedef FileRecord value_type;
	typedef off_t difference_type;
	typedef FileRecord* pointer;
	typedef FileRecord& reference;
	typedef std::random_access_iterator_tag iterator_category;

	FileIterator(char *base, size_t offset)
		: base(base), offset(offset)
	{ }

	FileIterator&
	operator+=(size_t off) {
		this->offset += off;
		return *this;
	}

	FileIterator
	operator++(int) {
		FileIterator t(*this);
		this->offset += 1;
		return t;
	}

	FileIterator&
	operator++() {
		this->offset += 1;
		return *this;
	}

	FileIterator&
	operator--() {
		this->offset -= 1;
		return *this;
	}

	FileIterator
	operator--(int) {
		FileIterator t(*this);
		this->offset -= 1;
		return t;
	}

	FileIterator&
	operator-=(size_t off) {
		assert(this->offset >= off);
		this->offset -= off;
		return *this;
	}

	bool
	operator<(FileIterator const &rhs) const {
		assert(this->base == rhs.base);
		return this->offset < rhs.offset;
	}

	bool
	operator==(FileIterator const &rhs) const {
		return !(*this < rhs) && !(rhs < *this);
	}

	bool
	operator!=(FileIterator const &rhs) const {
		return !(*this == rhs);
	}

	FileRecord&
	operator*() {
		return *reinterpret_cast<FileRecord*>(this->base + this->offset * szrecord);
	}

	FileRecord&
	operator[](size_t i) {
		*reinterpret_cast<FileRecord*>(this->base + (i + this->offset) * szrecord);
	}
};

off_t
operator-(FileIterator const &lhs, FileIterator const &rhs) {
	assert(lhs.base == rhs.base);
	if (rhs.offset > lhs.offset) {
		fprintf(stderr, "lhs.offset: %ld, rhs.offset: %ld\n", (long int)lhs.offset, (long int)rhs.offset);
	}
	assert(lhs.offset >= rhs.offset);
	return (lhs.offset - rhs.offset);
}

FileIterator
operator+(FileIterator lhs, off_t rhs) {
	lhs.offset += rhs;
	return lhs;
}

FileIterator
operator-(FileIterator lhs, off_t rhs) {
	lhs.offset -= rhs;
	return lhs;
}

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


int merge_sort(FileIterator start, FileIterator end, FileIterator bstart, FileIterator bend) {
	//fprintf(stderr, "merge_sort(%d, %d)\n", end-start, bend-bstart);
	if(end - start <= threshold) {
		std::sort(start, end);
	}
	else {
		size_t half = (end - start) / 2;
		cilk_spawn merge_sort(start, start+half, bstart, bstart + half);
		merge_sort(start + half, end, bstart + half, bend);
		cilk_sync;
		std::merge(start, start + half, start + half, end, bstart);
		std::copy(bstart, bend, start);
	}

	return 0;
}

int cilk_main(int argc, char ** argv) {
	char * path = NULL;
	int opt, flag_f = 0, flag_t = 0;
	szrecord = 100;
	
	while ((opt = getopt(argc, argv, "f:t:")) != -1) {
		switch(opt) {
		case 'f': path = optarg;
			flag_f = 1;  
			break;
		case 't': threshold = (off_t)(atoi(optarg)); // TODO: Fix me. Use atoll
			flag_t = 1;
			break;
			
		default:
			fprintf(stderr, "Usage: %s [-f filepath] [-t threshold]\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	
	if(!flag_f || !flag_t) {
		fprintf(stderr, "Usage: %s [-f filepath] [-t threshold]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	
	struct stat buf;
	if(stat(path, &buf)) {
		perror("Could not open file. ");
		return 1;
	}
	
	int fd = open(path, O_RDWR);
	nrecords = buf.st_size / szrecord;
	//printf("threshold:%d szrecord: %d nrecords: %d\n", threshold, szrecord, nrecords);
	assert(fd >= 0);
	assert(threshold >= 0 && szrecord > 0 && nrecords >=0);

	file_ptr = (char *) mmap(NULL, nrecords * szrecord, PROT_READ | \
		PROT_WRITE, MAP_SHARED|MAP_FILE, fd, 0);
	
	assert(file_ptr != (void*)-1);

	scratch = (char *) mmap(NULL, nrecords * szrecord, PROT_READ | PROT_WRITE, \
		MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	
	assert(scratch != (void*)-1);

	printf("sizeof(FileRecord): %d\n", sizeof(FileRecord));

	FileIterator start(file_ptr, 0), end(file_ptr, nrecords);
	FileIterator bstart(scratch, 0), bend(scratch, nrecords);
	// std::sort(start, end);

	// merge_sort(start, end, bstart, bend);
	parallel_merge_sort((FileRecord*)file_ptr, (FileRecord*)scratch, 0, nrecords, 0);
	return 0;
}