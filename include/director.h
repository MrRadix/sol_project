#define MIN_KTIME 20    // min time for a cashier in milliseconds
#define MAX_KTIME 80    // max time for a cashier in milliseconds


struct director_args {
    int n_clients;          // C
    int n_cashiers;         // K
    int n_client_group;     // E
    int product_time;       // time for handling a single product
    int client_max_time;    // T
    int n_max_product;      // P
    int def_cashiers_number;
    int s1;
    int s2;
};

struct clients_handler_args {
    int n_clients;
    int n_cashiers;
    int n_clients_group;
    int client_max_time;
    int n_max_products;
};

struct cashiers_handler_args {
    int n_clients;
    int n_cashiers;
    int product_time;
    int def_cashiers_number;
    int s1;
    int s2;
};

void *director(void *arg);