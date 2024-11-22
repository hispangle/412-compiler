#ifndef LIST_H
#define LIST_H
#include <stdlib.h>

//defines a linked list
typedef struct _List List;
struct _List{
    List* next;
    List* prev;
};

/*
 * Removes list_item from the linked list.
 * Assumes List is circularly doubly linked.
 * Does not remove next and prev fields of list_item.
 * Does nothing if list_item is NULL
 * 
 * Requires: List* list_item, the item of the list to be removed
 * Returns: nothing
*/
void remove_circularly_doubly(List* list_item){
    if(list_item == NULL) return;

    List* prev = list_item->prev;
    List* next = list_item->next;

    prev->next = next;
    next->prev = prev;
}

/*
 * Removes list_item from the linked list.
 * Assumes List is doubly linked.
 * Does not remove next and prev fields of list_item.
 * Does nothing if list_item is NULL
 * 
 * Requires: List* list_item, the item of the list to be removed
 * Returns: nothing
*/
void remove_doubly(List* list_item){
    if(list_item == NULL) return;

    List* prev = list_item->prev;
    List* next = list_item->next;

    if(prev != NULL) prev->next = next;
    if(next != NULL) next->prev = prev;
}

#endif