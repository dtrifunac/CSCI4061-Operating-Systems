#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

void list_init(list_t *list) {
    list->head = NULL;
    list->size = 0;
}

void list_add(list_t *list, const char* data) {
    // TODO
    node_t *ptr = malloc(sizeof(node_t));            //-> is to access a field of a struct if it is a pointer, use . if its teh actual struct
    strncpy(ptr->data,data,128);
    ptr->next = NULL;
    if(list->head == NULL){
        list->head = ptr;
    }
    else{
        node_t *h = list->head;
        while(h->next != NULL){
            h = h->next;
        }
        h->next = ptr;
    }
}

int list_size(const list_t *list) {
    // TODO
    int s = 1;
    if(list->head == NULL){
        return 0;
    }
    node_t *h = list->head;
    while(h->next != NULL){
        h = h->next;
        s++;
    }
    return s;
}

char *list_get(const list_t *list, int index) {
    // TODO
    if(index < 0 || index >= list->size){
        return NULL;
    }
    node_t *t = list->head;
    int i = 0;
    while(t != NULL && index != i){
        i++;
        t = t->next;
    }
    return t->data;
}

void list_clear(list_t* list) {
    // TODO
    if(list->head == NULL){
        return;
    }
    node_t *ptr = list->head;
    node_t *temp;
    while(ptr != NULL){
        temp = ptr;
        ptr = ptr->next;
        free(temp);
    }
}

int list_contains(const list_t *list, const char *query) {
    // TODO
    node_t *t = list->head;
    while(t != NULL){
        if(strcmp(query, t->data)==0){
            return 1;
        }
        t = t->next;
    }
    return 0;
}

void list_print(const list_t *list) {
    int i = 0;
    node_t *current = list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->data);
        current = current->next;
        i++;
    }
}
