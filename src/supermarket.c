#include "director.h"
#include "cashier.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#define CONF_FILENAME "config/config.txt"

void quit_handler(int signo) {
    quit = 1;
}

void hup_handler(int signo) {
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

    pthread_sigmask(SIG_SETMASK, &set, NULL);
}

void strstrip(char **string)
{
    char *filtered = (char*)malloc(strlen(*string)*sizeof(char));
    int i = 0;
    int j = strlen(*string) - 1;
    int k;
    int h = 0;

    strcpy(filtered, (*string));

    while (
        (*string)[i] == '\a' ||
        (*string)[i] == '\b' ||
        (*string)[i] == '\f' ||
        (*string)[i] == '\n' ||
        (*string)[i] == '\r' ||
        (*string)[i] == '\t' ||
        (*string)[i] == '\v')
    {
        i++;
    }

    while (
        (*string)[j] == '\a' ||
        (*string)[j] == '\b' ||
        (*string)[j] == '\f' ||
        (*string)[j] == '\n' ||
        (*string)[j] == '\r' ||
        (*string)[j] == '\t' ||
        (*string)[j] == '\v')
    {
        j--;
    }

    

    for (k = i; k <= j; k++)
    {
        (*string)[h++] = filtered[k];
    }
    (*string)[h] = '\0';

    free(filtered);
}

int main(int argc, char const *argv[])
{
    int k = -1;
    int init_k_n = -1;
    int c = -1;
    int e = -1;
    int t = -1;
    int p = -1;
    int prod_time = -1;
    int analytics_intervall = -1;
    int analytics_diff = -1;
    int s1 = -1;
    int s2 = -1;
    char *log_filename = NULL;

    pthread_t director_thread;
    
    FILE *config;
    char *line = NULL;
    size_t len = 0;
    size_t read;
    char *parameter = NULL;
    char *value = NULL;

    set_sig_handler();

    config = fopen(CONF_FILENAME, "r");
    if (config == NULL) {
        perror("error during config file open");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, config)) != -1) {

        parameter = strtok(line, " ");
        value = strtok(NULL, " ");

        strstrip(&parameter);
        strstrip(&value);

        if (strcmp(parameter, "K") == 0)                k = atoi(value);
        if (strcmp(parameter, "INITKN") == 0)           init_k_n = atoi(value);
        if (strcmp(parameter, "C") == 0)                c = atoi(value);
        if (strcmp(parameter, "E") == 0)                e = atoi(value);
        if (strcmp(parameter, "T") == 0)                t = atoi(value);
        if (strcmp(parameter, "P") == 0)                p = atoi(value);
        if (strcmp(parameter, "PRODTIME") == 0)         prod_time = atoi(value);
        if (strcmp(parameter, "ANALYTICS_T") == 0)      analytics_intervall = atoi(value);
        if (strcmp(parameter, "ANALYTICS_DIFF") == 0)   analytics_diff = atoi(value);
        if (strcmp(parameter, "S1") == 0)               s1 = atoi(value);
        if (strcmp(parameter, "S2") == 0)               s2 = atoi(value);
        if (strcmp(parameter, "LOG_FN") == 0) {
            log_filename = (char*)malloc(strlen(value)*sizeof(char));
            strcpy(log_filename, value);
        }

        free(line);
        line = NULL;
        len = 0;
    }
    free(line);

    fclose(config);

    /**
     * check of parameters values
     */
    if (k < 1) {
        fprintf(stderr, "K must be > 1\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (init_k_n < 1 || init_k_n > k) {
        fprintf(stderr, "INITKN mut be set between 1 and K\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (c < 1) {
        fprintf(stderr, "C must be set greather than 1\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (e < 1 || e > c) {
        fprintf(stderr, "E mut be set between 1 and C\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (t <= 10) {
        fprintf(stderr, "T mut be set greater than 10\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (p < 0) {
        fprintf(stderr, "P must be set greater or equal than 0\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (prod_time <= 0) {
        fprintf(stderr, "PRODTIME must be set greater than 0\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (analytics_intervall <= 0) {
        fprintf(stderr, "ANALYTICS_T must be set greater than 0\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (analytics_diff <= 0) {
        fprintf(stderr, "ANALYTICS_DIFF must be set greater than 0\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (log_filename == NULL || strcmp(log_filename, "") == 0) {
        fprintf(stderr, "LOG_FN need to be set\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (s1 < 1 || s1 > k) {
        fprintf(stderr, "S1 must be set between 1 and K\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }
    if (s2 < 1 || s2 > c) {
        fprintf(stderr, "S2 must be between 1 and C\n");
        free(log_filename);
        exit(EXIT_FAILURE);
    }


    struct director_args *d_args = (struct director_args *)malloc(sizeof(struct director_args));
    d_args->log_file_name = (char*)malloc(strlen(log_filename)*sizeof(char));
    strncpy(d_args->log_file_name, log_filename, strlen(log_filename));
    d_args->client_max_time = t;
    d_args->def_cashiers_number = init_k_n;
    d_args->n_cashiers = k;
    d_args->n_clients = c;
    d_args->n_client_group = e;
    d_args->n_max_product = p;
    d_args->product_time = prod_time;
    d_args->analytics_t_intervall = analytics_intervall;
    d_args->analytics_time_diff = analytics_diff;
    d_args->s1 = 4;
    d_args->s2 = 10;

    pthread_create(&director_thread, NULL, director, (void *)d_args);

    pthread_join(director_thread, NULL);

    free(d_args->log_file_name);
    free(d_args);
    free(log_filename);

    return 0;
}
