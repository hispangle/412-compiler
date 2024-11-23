#ifndef LIST_H
#define LIST_H

//typedef declarations
typedef struct List List;

//function declarations
inline static void remove_from_list(List* list_item);
inline static void add_to_list(List* list_head, List* item);

//linked list
struct List{
    List* next;
    List* prev;
};


//inline functions declared in header
/*
 * Removes list_item from the linked list.
 * Assumes List is circularly doubly linked.
 * Does not remove next and prev fields of list_item.
 * Does nothing if list_item is NULL
 * 
 * Requires: List* list_item, the item of the list to be removed
 * Returns: nothing
*/
inline static void remove_from_list(List* list_item){
    List* prev = list_item->prev;
    List* next = list_item->next;

    prev->next = next;
    next->prev = prev;
    return;
}

/*
*/
inline static void add_to_list(List* list_head, List* item){
    list_head->prev->next = item;
    item->prev = list_head->prev;
    item->next = list_head;
    list_head->prev = item;
    return;
}


#endif