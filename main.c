#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "gol.h"

// #define DEBUG 1

// n_threads: 3
// Solução concorrente 1: 675 sec (criando threads a cada loop)
// Solução concorrente 2: 605 sec (criando threads só no início)
// Solução sequencial: 994 sec
// speedup: 1.64
// Eficiência: 54,6 %



int main(int argc, char **argv)
{
    int size, steps, n_threads;
    cell_t **curr, **next, **tmp;
    FILE *f;
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
    sem_t sem_round_finished, threads_finished_lock;

    sem_init(&sem_round_finished, 0, 0);
    sem_init(&threads_finished_lock, 0, 1);

    read_file(f, curr, size);

    fclose(f);

#ifdef DEBUG
    printf("Initial:\n");
    print_board(curr, size);
    print_stats(stats_total);
#endif
    
    // criação das threads
    unsigned int thread_size = size / n_threads;
    unsigned int threads_finished = 0;
    for (int i = 0; i < n_threads; i++) {
        threads_arg[i].begin = i * thread_size;
        threads_arg[i].end = (i < n_threads - 1) ? threads_arg[i].begin + thread_size : size; // Ultima thread fica responsavel com o restante das linhas da matriz
        threads_arg[i].size = size;
        threads_arg[i].curr = curr;
        threads_arg[i].next = next;
        threads_arg[i].semaphore = &semaphores[i];
        threads_arg[i].threads_finished = &threads_finished;
        threads_arg[i].threads_finished_lock = &threads_finished_lock;
        threads_arg[i].sem_round_finished = &sem_round_finished;
        threads_arg[i].steps = steps;
        threads_arg[i].n_threads = n_threads;

        sem_init(&semaphores[i], 0, 1);
        
        pthread_create(&threads[i], NULL, play_round, &threads_arg[i]);
    }

    for (int _ = 0; _ < steps; _++)
    {

        /*
        - A thread principal espera até que todas as threads terminem o round atual.
        - Quando isso acontece, a thread principal incremental o semáforo das outras threads
          para sinalizar o começo de um novo round
        */
        sem_wait(&sem_round_finished);
        for ( int i = 0; i < n_threads; ++i )
            sem_post(&semaphores[i]);

#ifdef DEBUG
        print_board(next, size);
        print_stats(stats_total);
#endif
        tmp = next;
        next = curr;
        curr = tmp;
    }

    for ( int i = 0; i < n_threads; ++i) {
        stats_t *stats_returned;
        pthread_join(threads[i], (void *)& stats_returned);
        stats_total.borns += stats_returned->borns;
        stats_total.survivals += stats_returned->survivals;
        stats_total.loneliness += stats_returned->loneliness;
        stats_total.overcrowding += stats_returned->overcrowding;
        
        sem_destroy(&(semaphores[i]));

        free(stats_returned);
    }

    // Destroi os semaforos
    sem_destroy(&sem_round_finished);
    sem_destroy(&threads_finished_lock);

#ifdef RESULT
    printf("Final:\n");
    print_board(curr, size);
    print_stats(stats_total);
#endif

    free_board(curr, size);
    free_board(next, size);
}
