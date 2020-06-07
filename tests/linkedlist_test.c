#include "linkedlist.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int_list_node *list = NULL;
    int_list_node *rest = NULL;

    printf("* adding: ");
    for (int i = 0; i<10; i++) {
        printf("%d ", i);
        add(&list, i);
    }
    printf("elements...\n\n");

    printf("* testing next function\nexpected output: 0 1 2 3 4 5 6 7 8 9\n\n");

    printf("%d ", next(list));
    for (int i = 1; i<10; i++) {
        printf("%d ", next(NULL));
    }
    printf("\n\n");

    printf("* testing next_r function\nexpected output: 0 1 2 3 4 5 6 7 8 9\n\n");
    
    printf("%d ", next_r(list, &rest));
    for (int i = 1; i<10; i++) {
        printf("%d ", next_r(NULL, &rest));
    }
    printf("\n\n");

    printf("*testing len function\nexpected output: 10\n\n");

    printf("%d\n", len(list));

}