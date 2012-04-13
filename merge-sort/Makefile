CXXFLAGS := $(CXXFLAGS) -O2 -Wall

all: merge-sort drop-caches bottom-up median-merge-sort include.hpp median_merge_sort.hpp is_sorted.hpp

drop-caches: drop_caches.c
	gcc -o drop_caches drop_caches.c
	# sudo chown root ./drop_caches
	# sudo chmod +s ./drop_caches

merge-sort: merge_sort.cpp merge_fast.hpp
	cilk++ merge_sort.cpp -o merge_sort $(CXXFLAGS)

median-merge-sort: median_merge_sort.cpp
	cilk++ median_merge_sort.cpp -o median_merge_sort $(CXXFLAGS)

bottom-up: bottom_up_merge_sort-1.cpp bottom_up_merge_sort-2.cpp
	$(CXX) bottom_up_merge_sort-1.cpp -o bottom_up-1 $(CXXFLAGS)
	$(CXX) bottom_up_merge_sort-2.cpp -o bottom_up-2 $(CXXFLAGS)


clean:
	rm -f merge_sort median_merge_sort bottom_up-1 bottom_up-2 drop_caches
