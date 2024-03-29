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

#define BUFSZ 131072 

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
	
	nrecords = buf.st_size / szrecord;
	//printf("threshold:%d szrecord: %d nrecords: %d\n", threshold, szrecord, nrecords);
	assert(threshold >= 0 && szrecord > 0 && nrecords >=0);
    
    file_ptr = new char[nrecords * szrecord];
    assert(file_ptr != NULL);

    scratch = new char[nrecords * szrecord];
    assert(scratch != NULL);
    
    FILE * fp = fopen(path, "r");
    
    int bytes_read = 0, i = 0;
    while((bytes_read = fread(file_ptr + i, sizeof(char), BUFSZ, fp)) > 0) {
        i += bytes_read;
    }

    //for(int i = 0; i < buf.st_size; i++)
    //    fscanf(fp, "%c", &file_ptr[i]);
    fclose(fp);

	printf("sizeof(FileRecord): %d\n", sizeof(FileRecord));

	FileIterator start(file_ptr, 0), end(file_ptr, nrecords);
	FileIterator bstart(scratch, 0), bend(scratch, nrecords);
	// std::sort(start, end);

	merge_sort(start, end, bstart, bend);
    
    fp = fopen(path, "wb");
    fwrite(file_ptr, sizeof(char), buf.st_size, fp);
    fclose(fp);
	return 0;
}

