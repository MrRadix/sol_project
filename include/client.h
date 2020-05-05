#include <time.h>

struct client_args {
    int id;         // customer id
    time_t p_time;  // time for purchases in ms
    int max_cash;
    int products;   // products number to buy
};

void *client(void *arg);

