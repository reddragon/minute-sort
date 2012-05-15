/* -*- Mode: C++; c-basic-offset: 8; indent-tabs-mode: t -*- */

#include "include.hpp"
#include <stdint.h>

off_t nrecords, szrecord;
char *file_ptr;

struct FileRecord {
    mutable char base[100];

    void
    set_data(uint64_t data) {
        uint64_t *pd = (uint64_t*)(base + 10);
        *pd = data;
    }

};

int main(int argc, char ** argv) {
    char * path = NULL;
    int opt, flag_f = 0;
    szrecord = 100;

    while ((opt = getopt(argc, argv, "f:t:")) != -1) {
        switch(opt) {
        case 'f': path = optarg;
            flag_f = 1;  
            break;
			
        default:
            fprintf(stderr, "Usage: %s [-f filepath]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
	
    if(!flag_f) {
        fprintf(stderr, "Usage: %s [-f filepath]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	
	
    struct stat buf;
    if(stat(path, &buf)) {
        perror("Could not open file. ");
        return 1;
    }
	
    int fd = open(path, O_RDWR);
    nrecords = buf.st_size / szrecord;
    assert(fd >= 0);
    assert(nrecords >=0);

    file_ptr = (char *) mmap(NULL, nrecords * szrecord, PROT_READ |     \
                             PROT_WRITE, MAP_SHARED|MAP_FILE, fd, 0);
	
    assert(file_ptr != (void*)-1);

    FileRecord *fr = (FileRecord*)file_ptr;

    for (size_t i = 0; i < nrecords; ++i) {
        fr[i].set_data(i);
    }

    return 0;
}

