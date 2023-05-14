#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "gol.h"

// #define DEBUG 1

int main(int argc, char **argv)
{
    int size, steps, n_threads;
    cell_t **curr, **next, **tmp;
    FILE *f;
    // stats_t stats_step = {0, 0, 0, 0};
    stats_t stats_total = {0, 0, 0, 0};

    if (argc != 3)
    {
        // printf("ERRO! Você deve digitar %s <nome do arquivo do tabuleiro>!\n\n", argv[0]);
        printf("Uso: main <nome do arquivo> <número de threads>\n\n");
        return 0;
    }

    if ((f = fopen(argv[1], "r")) == NULL)
    {
        printf("ERRO! O arquivo de tabuleiro '%s' não existe!\n\n", argv[1]);
        return 0;
    }

    n_threads = atoi(argv[2]);
    fscanf(f, "%d %d", &size, &steps);

    if (n_threads > size){
        n_threads = size;
    }

    curr = allocate_board(size);
    next = allocate_board(size);
    pthread_t threads[n_threads];
    thread_arguments threads_arg[n_threads];
    sem_t semaphores[n_threads];
    sem_t sem_finish_round;

    read_file(f, curr, size);

    fclose(f);

#ifdef DEBUG
    printf("Initial:\n");
    print_board(curr, size);
    print_stats(stats_total);
#endif

    int thread_size = size / n_threads;
    for (int _ = 0; _ < steps; _++)
    {
        printf("Step %d\n\n", _);
        for (int i = 0; i < n_threads - 1; i++){
            threads_arg[i].begin = i * thread_size;
            threads_arg[i].end = threads_arg[i].begin + thread_size;
            threads_arg[i].size = size;
            threads_arg[i].curr = curr;
            threads_arg[i].next = next;
            
            pthread_create(&threads[i], NULL, play_round, &threads_arg[i]);
        }

        // Create last thread
        int i = n_threads - 1;
        threads_arg[i].begin = i * thread_size;
        threads_arg[i].end = size;
        threads_arg[i].size = size;
        threads_arg[i].curr = curr;
        threads_arg[i].next = next;
        pthread_create(&threads[i], NULL, play_round, &threads_arg[i]);

        

        for ( int i = 0; i < n_threads; ++i) {
            stats_t *stats_returned;
            pthread_join(threads[i], (void *)& stats_returned);
            stats_total.borns += stats_returned->borns;
            stats_total.survivals += stats_returned->survivals;
            stats_total.loneliness += stats_returned->loneliness;
            stats_total.overcrowding += stats_returned->overcrowding;
            
            free(stats_returned);
        }
        

#ifdef DEBUG
        printf("Step %d ----------\n", i + 1);
        print_board(next, size);
        print_stats(stats_total);
#endif
        tmp = next;
        next = curr;
        curr = tmp;
    }

#ifdef RESULT
    printf("Final:\n");
    print_board(curr, size);
    print_stats(stats_total);
#endif

    free_board(curr, size);
    free_board(next, size);
}
