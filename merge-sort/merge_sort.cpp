/* -*- Mode: C++; c-basic-offset: 8; indent-tabs-mode: t -*- */

#include "include.hpp"
#include "median_merge_sort.hpp"
#include "is_sorted.hpp"
#include "merge_fast.hpp"

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

#define CHARCMP(LHS, RHS, I) if (((unsigned char*)LHS)[I] < ((unsigned char*)RHS)[I]) return true; if (((unsigned char*)LHS)[I] > ((unsigned char*)RHS)[I]) return false;

inline bool
fast_keycmp(const char *lhs, const char *rhs) {
	CHARCMP(lhs, rhs, 0);
	CHARCMP(lhs, rhs, 1);
	CHARCMP(lhs, rhs, 2);
	CHARCMP(lhs, rhs, 3);
	CHARCMP(lhs, rhs, 4);
	CHARCMP(lhs, rhs, 5);
	CHARCMP(lhs, rhs, 6);
	CHARCMP(lhs, rhs, 7);
	CHARCMP(lhs, rhs, 8);
	CHARCMP(lhs, rhs, 9);
	return false;
}

using namespace std;

char *file_ptr, *scratch;

struct FileRecord {
	mutable char base[100];

	bool
	operator<(FileRecord const &rhs) const {
		// this->base[99] = '\0';
		// rhs.base[99] = '\0';
#if 0
		uint64_t *pl = (uint64_t*)this->base;
		uint64_t *pr = (uint64_t*)rhs.base;

		if (*pl < *pr) {
			return true;
		}
		if (*pl > *pr) {
			return false;
		}
		// They are equal.
		pl = (uint64_t*)(this->base+2);
		pr = (uint64_t*)(rhs.base+2);
		if (*pl < *pr) {
			return true;
		}
		return false;
#else
		// const bool res = memcmp(this->base, rhs.base, 10) < 0;
		const bool res = fast_keycmp(this->base, rhs.base);

		// fprintf(stderr, "%s is %s %s\n", this->base, (res ? "<" : ">="), rhs.base);
		return res;
#endif
	}

	bool
	operator>(FileRecord const &rhs) const {
		return rhs < *this;
	}

	bool
	operator<=(FileRecord const &rhs) const {
		const bool res = (*this < rhs) || (*this == rhs);
		return res;
	}

	bool
	operator>=(FileRecord const &rhs) const {
		const bool res = (rhs < *this) || (*this == rhs);
		return res;
	}

	bool
	operator==(FileRecord const &rhs) const {
		const bool res = !(*this < rhs) && !(rhs < *this);
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
	operator>(FileIterator const &rhs) const {
		return rhs < *this;
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
		return *reinterpret_cast<FileRecord*>(this->base + (i + this->offset) * szrecord);
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

ostream&
operator<<(ostream &out, FileRecord const &data) {
	char key[11];
	memcpy(key, data.base, 10);
	key[10] = '\0';
	out<<key;
	return out;
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
		merge_fast(&*start, &start[half], &start[half], &end[0], &bstart[0]);
		// std::merge(start, start + half, start + half, end, bstart);
		// std::copy(bstart, bend, start);
	}

	return 0;
}

int MAIN(int argc, char ** argv) {
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

#if 0
	// Try to just read in every entry.
	/*
	int hash = 0;
	for (off_t i = 0; i < nrecords; ++i) {
		hash += file_ptr[i*szrecord];
		file_ptr[i*szrecord] = i;
	}
	printf("hash: %d\n", hash);
	*/
	merge_sort(start, end, bstart, bend);
#else
	parallel_merge_sort((FileRecord*)file_ptr, (FileRecord*)scratch, 0, nrecords, 0);
#endif
	// assert(is_sorted((FileRecord*)file_ptr, (FileRecord*)file_ptr + nrecords));
	return 0;
}
