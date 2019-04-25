#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "treeset.h"
#include "factor_counts.h"

static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
static TreeSet *treeset;

struct thread_data {
	int low;
	int high;
	int output_length;
};

/// returns the number of factors that num has
int count_factors(int num) {
    return factor_counts[num-1];
}

/// this is meant for use with pthread_create, arg should be a thread_data
void *find_max_factors(void *arg) {
	struct thread_data *input = (struct thread_data *) arg;
	//printf("Low: %d High: %d\n", input->low, input->high);
	for (int i = input->low; i < input->high; i++) {
		pthread_rwlock_rdlock(&lock);
		if (treeset_size(treeset) < input->output_length) {
			// tree does not yet contain max number of nodes
			pthread_rwlock_unlock(&lock);
			pthread_rwlock_wrlock(&lock);// now we want a write lock to swap out a node
			treeset_add(treeset, (void *) (intptr_t) i);
			pthread_rwlock_unlock(&lock);
		} else {
			void *leftmost = 0;
			treeset_get_first(treeset, &(leftmost));
			pthread_rwlock_unlock(&lock);// done reading for now
			if (count_factors(i) > (count_factors((int) (intptr_t) leftmost))) {
				// add if i has more factors than leftmost node
				pthread_rwlock_wrlock(&lock);
				treeset_remove(treeset, leftmost, NULL);
				treeset_add(treeset, (void *) (intptr_t) i);
				pthread_rwlock_unlock(&lock);
			}
		}
	}
	pthread_exit(arg);
	return NULL;
}

/// This is for comparing nodes in the treeset
int compare_dividends(const void *void1, const void *void2) {
	int dividend1 = (int)(intptr_t) void1;
	int dividend2 = (int)(intptr_t) void2;
	int difference = count_factors(dividend1) - count_factors(dividend2);
	if (difference != 0) {
		return difference;
	} else {
		return dividend1 - dividend2;
	}
}

/// start program
int main(int argc, char **argv) {
	if (argc < 5) {
		// too few program arguments
		return -1;
	} else {
		time_t start = time(NULL);
		int range_start = atoi(argv[1]);
		int range_end = atoi(argv[2]);
		int output_length = atoi(argv[3]);
		int thread_count = atoi(argv[4]);
		if (thread_count > range_end - range_start + 1) {
			thread_count = range_end - range_start + 1;
		}
		pthread_t threads[thread_count];
		struct thread_data thread_args[thread_count];
		treeset_new(&compare_dividends, &treeset);
		for (int i = 0; i < thread_count; i++) {
			//thread_args[i].low = range_start + i * (range_end - range_start) / thread_count;
			//thread_args[i].high = thread_args[i].low + (range_end - range_start) / thread_count;
			thread_args[i].low = range_start + i;
			thread_args[i].high = thread_args[i].low + 1;
			thread_args[i].output_length = output_length;

			if (i == thread_count - 1) {
				// last iteration
				thread_args[i].high = range_end + 1;
			}
			pthread_create(&threads[i], NULL, find_max_factors, (void*) &thread_args[i]);
		}
		// wait for threads to finish
		for (int i = 0; i < thread_count; i++) {
			pthread_join(threads[i], NULL);
		}
		// no other threads so no need for lock
		int highest_factor;
		for (int i = 0; i < output_length; i++) {
			treeset_get_last(treeset, (void **)(&highest_factor));
			printf("%d, factors: %d\n", highest_factor, count_factors(highest_factor));
			treeset_remove(treeset, (void *) (intptr_t) highest_factor, NULL);
		}
		time_t stop;
		time(&stop);
		double elapsed = difftime(stop, start);
		printf("%.f sec\n", elapsed);
		//printf("%f\t%f", start, stop);
		return 0;
	}
}
