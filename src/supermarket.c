#include "director.h"
#include "cashier.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#define K 100         // cashers number
#define INITKN 99     // initial cashiers number INITKN > 0
#define C 500        // customers number
#define E 100          // customers every group 0 < E < C
#define T 11            // max time for purchases in milliseconds T > 10
#define P 1           // max number of product for each customer P > 0

/**
 * time in milliseconds to process every product
 */
#define PRODTIME 50

#define ANALYTICS_INTERVAL 100


void quit_handler(int signo) {
    fprintf(stderr, "received %d SIGQUIT", signo);
    quit = 1;
}

void hup_handler(int signo) {
    fprintf(stderr, "received %d SIGHUP", signo);
    closing = 1;
}

void set_sig_handler(void) {
    sigset_t set;
    struct sigaction action;

    if (sigfillset(&set) != 0) {
        perror("error during filling set");
        exit(EXIT_FAILURE);
    }

    memset(&action, 0, sizeof(action));

    action.sa_handler = quit_handler;
    if(sigaction(SIGQUIT, &action, NULL) != 0) {
        perror("error during set sigaction");
        exit(EXIT_FAILURE);
    }

    action.sa_handler = hup_handler;
    if(sigaction(SIGHUP, &action, NULL) != 0) {
        perror("error during set sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigfillset(&set) != 0) {
        perror("error during filling set");
        exit(EXIT_FAILURE);
    }

    if (sigdelset(&set, SIGQUIT) != 0) {
        perror("error during unmasking SIGQUIT");
        exit(EXIT_FAILURE);
    }

    if (sigdelset(&set, SIGHUP) != 0) {
        perror("error during unmasking HUP");
        exit(EXIT_FAILURE);
    }
    
    //pthread_sigmask(SIG_SETMASK, &set, NULL);
}

int main(int argc, char const *argv[])
{
    pthread_t director_thread;

    set_sig_handler();

    struct director_args *d_args = (struct director_args *)malloc(sizeof(struct director_args));
    d_args->client_max_time = T;
    d_args->def_cashiers_number = INITKN;
    d_args->n_cashiers = K;
    d_args->n_clients = C;
    d_args->n_client_group = E;
    d_args->n_max_product = P;
    d_args->product_time = PRODTIME;
    d_args->analytics_t_intervall = ANALYTICS_INTERVAL;
    d_args->s1 = 4;
    d_args->s2 = 10;

    pthread_create(&director_thread, NULL, director, (void *)d_args);

    pthread_join(director_thread, NULL);

    free(d_args);

    return 0;
}