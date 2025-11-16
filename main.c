#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>

typedef struct {
    int* array;
    int low;
    int count;
    int direction;
    int depth;
} sort_params_t;

pthread_mutex_t thread_mutex;
int active_threads = 0;
int max_threads = 4;

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void bitonic_compare(int* array, int low, int count, int direction) {
    if (count <= 1) return;
    
    int k = count / 2;
    for (int i = low; i < low + k; i++) {
        if (direction == (array[i] > array[i + k])) {
            swap(&array[i], &array[i + k]);
        }
    }
}

void* bitonic_sort(void* params) {
    sort_params_t* sp = (sort_params_t*)params;
    int* array = sp->array;
    int low = sp->low;
    int count = sp->count;
    int direction = sp->direction;
    int depth = sp->depth;

    if (count <= 1) {
        return NULL;
    }

    int k = count / 2;

    bitonic_compare(array, low, count, direction);

    sort_params_t left_params = {array, low, k, direction, depth + 1};
    sort_params_t right_params = {array, low + k, k, direction, depth + 1};

    pthread_t left_thread, right_thread;
    int left_created = 0, right_created = 0;

    if (k > 1 && depth < 3) {
        pthread_mutex_lock(&thread_mutex);
        if (active_threads < max_threads) {
            active_threads++;
            pthread_mutex_unlock(&thread_mutex);
            
            if (pthread_create(&left_thread, NULL, bitonic_sort, &left_params) == 0) {
                left_created = 1;
            } else {
                pthread_mutex_lock(&thread_mutex);
                active_threads--;
                pthread_mutex_unlock(&thread_mutex);
            }
        } else {
            pthread_mutex_unlock(&thread_mutex);
        }
    }

    if (k > 1 && depth < 3) {
        pthread_mutex_lock(&thread_mutex);
        if (active_threads < max_threads) {
            active_threads++;
            pthread_mutex_unlock(&thread_mutex);
            
            if (pthread_create(&right_thread, NULL, bitonic_sort, &right_params) == 0) {
                right_created = 1;
            } else {
                pthread_mutex_lock(&thread_mutex);
                active_threads--;
                pthread_mutex_unlock(&thread_mutex);
            }
        } else {
            pthread_mutex_unlock(&thread_mutex);
        }
    }

    if (!left_created) {
        bitonic_sort(&left_params);
    }
    if (!right_created) {
        bitonic_sort(&right_params);
    }

    if (left_created) {
        pthread_join(left_thread, NULL);
        pthread_mutex_lock(&thread_mutex);
        active_threads--;
        pthread_mutex_unlock(&thread_mutex);
    }
    if (right_created) {
        pthread_join(right_thread, NULL);
        pthread_mutex_lock(&thread_mutex);
        active_threads--;
        pthread_mutex_unlock(&thread_mutex);
    }

    return NULL;
}

void* bitonic_merge(void* params) {
    sort_params_t* sp = (sort_params_t*)params;
    int* array = sp->array;
    int low = sp->low;
    int count = sp->count;
    int direction = sp->direction;
    int depth = sp->depth;

    if (count <= 1) {
        return NULL;
    }

    int k = count / 2;

    sort_params_t left_params = {array, low, k, 1, depth + 1};
    sort_params_t right_params = {array, low + k, k, 0, depth + 1};

    pthread_t left_thread, right_thread;
    int left_created = 0, right_created = 0;

    if (k > 1 && depth < 2) {
        pthread_mutex_lock(&thread_mutex);
        if (active_threads < max_threads) {
            active_threads++;
            pthread_mutex_unlock(&thread_mutex);
            
            if (pthread_create(&left_thread, NULL, bitonic_merge, &left_params) == 0) {
                left_created = 1;
            } else {
                pthread_mutex_lock(&thread_mutex);
                active_threads--;
                pthread_mutex_unlock(&thread_mutex);
            }
        } else {
            pthread_mutex_unlock(&thread_mutex);
        }
    }

    if (k > 1 && depth < 2) {
        pthread_mutex_lock(&thread_mutex);
        if (active_threads < max_threads) {
            active_threads++;
            pthread_mutex_unlock(&thread_mutex);
            
            if (pthread_create(&right_thread, NULL, bitonic_merge, &right_params) == 0) {
                right_created = 1;
            } else {
                pthread_mutex_lock(&thread_mutex);
                active_threads--;
                pthread_mutex_unlock(&thread_mutex);
            }
        } else {
            pthread_mutex_unlock(&thread_mutex);
        }
    }

    if (!left_created) {
        bitonic_merge(&left_params);
    }
    if (!right_created) {
        bitonic_merge(&right_params);
    }

    if (left_created) {
        pthread_join(left_thread, NULL);
        pthread_mutex_lock(&thread_mutex);
        active_threads--;
        pthread_mutex_unlock(&thread_mutex);
    }
    if (right_created) {
        pthread_join(right_thread, NULL);
        pthread_mutex_lock(&thread_mutex);
        active_threads--;
        pthread_mutex_unlock(&thread_mutex);
    }

    sort_params_t merge_params = {array, low, count, direction, depth};
    bitonic_sort(&merge_params);

    return NULL;
}

void bitonic_main_sort(int* array, int size) {
    int next_power = 1;
    while (next_power < size) {
        next_power <<= 1;
    }
    
    if (next_power != size) {
        int* temp_array = malloc(next_power * sizeof(int));
        memcpy(temp_array, array, size * sizeof(int));
        
        for (int i = size; i < next_power; i++) {
            temp_array[i] = INT_MAX;
        }
        
        sort_params_t params = {temp_array, 0, next_power, 1, 0};
        bitonic_merge(&params);
        
        memcpy(array, temp_array, size * sizeof(int));
        free(temp_array);
    } else {
        sort_params_t params = {array, 0, size, 1, 0};
        bitonic_merge(&params);
    }
}

int is_sorted(int* array, int size) {
    for (int i = 1; i < size; i++) {
        if (array[i] < array[i - 1]) {
            return 0;
        }
    }
    return 1;
}

void print_array(int* array, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <array_size> <max_threads> [--print]\n", argv[0]);
        return 1;
    }

    int array_size = atoi(argv[1]);
    max_threads = atoi(argv[2]);
    int print_array_flag = 0;

    if (argc > 3 && strcmp(argv[3], "--print") == 0) {
        print_array_flag = 1;
    }

    if (pthread_mutex_init(&thread_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        return 1;
    }

    int* array = malloc(array_size * sizeof(int));
    if (array == NULL) {
        perror("malloc");
        return 1;
    }

    srand(time(NULL));
    for (int i = 0; i < array_size; i++) {
        array[i] = rand() % 1000;
    }

    printf("Bitonic sort: %d elements, max threads: %d\n", array_size, max_threads);
    printf("PID: %d\n", getpid());
    printf("Run in another terminal: watch 'ps -eLf | grep %s | grep -v grep'\n", argv[0]);

    if (print_array_flag && array_size <= 50) {
        printf("Original array:\n");
        print_array(array, array_size);
    }

    double start_time = get_current_time();

    pthread_mutex_lock(&thread_mutex);
    active_threads++;
    pthread_mutex_unlock(&thread_mutex);

    bitonic_main_sort(array, array_size);

    pthread_mutex_lock(&thread_mutex);
    active_threads--;
    pthread_mutex_unlock(&thread_mutex);

    double end_time = get_current_time();

    if (is_sorted(array, array_size)) {
        printf("Successfully sorted in %.6f seconds\n", end_time - start_time);
    } else {
        printf("Sorting failed\n");
    }

    if (print_array_flag && array_size <= 50) {
        printf("Sorted array:\n");
        print_array(array, array_size);
    }

    free(array);
    pthread_mutex_destroy(&thread_mutex);

    return 0;
}