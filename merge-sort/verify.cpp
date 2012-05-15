#include <string.h>
#include <math.h>
#include <assert.h>
#include "include.hpp"
#include <stdint.h>

using namespace std;

vector<bool> bitmap;
uint64_t count_left;

void 
init_bitmap(off_t records) {
    // We can store ? entries in a uint64_t
    bitmap.resize(records);
    count_left = records;
};

void 
make_entry(const unsigned char* rec) {
    uint64_t record = *((uint64_t*)(rec+10));
    assert(record < bitmap.size());
    assert (!bitmap[record]);
    bitmap[record] = 1;
    count_left--;
}

void 
check_count() {
    assert(count_left == 0);
}

off_t nrecords, szrecord;
char *file_ptr;

struct FileRecord {
    const unsigned char base[100];
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
    
    init_bitmap(nrecords);
    for (size_t i = 0; i < nrecords; ++i) {
        make_entry(fr[i].base);
    }
    check_count();
    printf("SUCCESS - all records are unique\n");
    return 0;
}

    
