#include "linkedlist.h"
#include <stdlib.h>

int next(int_list_node *list) {
    static int_list_node *rest = NULL;
    int ret;

    if (list == NULL && rest == NULL) {
        return -1;
    }

    if (list == NULL) {
        ret = rest->el;
        rest = rest->next;
    } else {
        ret = list->el;
        rest = list->next;
    }

    return ret;
}

int next_r(int_list_node *list, int_list_node **rest) {
    int ret;

    if (list == NULL && rest == NULL) {
        return -1;
    }

    if (list == NULL) {
        ret = (*rest)->el;
        (*rest) = (*rest)->next;
    } else {
        ret = list->el;
        (*rest) = list->next;
    }

    return ret;
}

void add(int_list_node **list, int element) {
    int_list_node *tmp_lst = *list;

    if (tmp_lst == NULL) {
        (*list) = (int_list_node *)malloc(sizeof(int_list_node));
        (*list)->el = element;
        (*list)->next = NULL;
    } else {

        while (tmp_lst->next) {
            tmp_lst = tmp_lst->next;
        }

        tmp_lst->next = (int_list_node *)malloc(sizeof(int_list_node));
        tmp_lst = tmp_lst->next;
        tmp_lst->el = element;
        tmp_lst->next = NULL;
    }
}

int len(int_list_node *list) {
    int_list_node *tmp_lst = list;
    int length = 0;

    while (tmp_lst) {
        length++;
        tmp_lst = tmp_lst->next;
    }

    return length;
}

void destroy(int_list_node *list) {
    void *victim;

    while (list) {
        victim = list;
        list = list->next;
        free(victim);
    }
}