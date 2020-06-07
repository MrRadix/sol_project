
typedef struct _int_list_node {
    long el;
    struct _int_list_node * next;
} int_list_node;

/**
 * if list is not NULL returns first element of list
 * else returns next elements of list
 */
int next(int_list_node *list);

/**
 * reentrant version of next
 */
int next_r(int_list_node *list, int_list_node **rest);

/**
 * adds element at the end of list
 */
void add(int_list_node **list, int element);

/**
 * returns len of list
 */
int len(int_list_node *list);

/**
 * frees all data in list
 */
void destroy(int_list_node *list);