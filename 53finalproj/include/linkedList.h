#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "structs.h"
#include "protocol.h"
/*
 * Structre for each node of the linkedList
 *
 * value - a pointer to the data of the node. 
 * next - a pointer to the next node in the list. 
 */
typedef struct node {
    void* value;
    struct node* next;
} node_t;

/*
 * Structure for the base linkedList
 * 
 * head - a pointer to the first node in the list. NULL if length is 0.
 * length - the current length of the linkedList. Must be initialized to 0.
 * comparator - function pointer to linkedList comparator. Must be initialized!
 */
typedef struct list {
    node_t* head;
    int length;
    /* the comparator uses the values of the nodes directly (i.e function has to be type aware) */
    int (*comparator)(void*, void*);
} List_t;



int findbyname(List_t* listHead, char * username);
int findbyroom(List_t* listHead, char * roomname);
int findroomindex(List_t* listHead, char * roomname);
void printUserList(List_t* listHead);
void printJobList(List_t* listHead);
/* 
 * Each of these functions inserts the reference to the data (valref)
 * into the linkedList list at the specified position
 *
 * @param list pointer to the linkedList struct
 * @param valref pointer to the data to insert into the linkedList
 */
void insertRear(List_t* list, void* valref);
void insertFront(List_t* list, void* valref);
void insertInOrder(List_t* list, void* valref);

/*
 * Each of these functions removes a single linkedList node from
 * the LinkedList at the specfied function position.
 * @param list pointer to the linkedList struct
 * @return a pointer to the removed list node
 */ 
void* removeFront(List_t* list);
void* removeRear(List_t* list);
void* removeByIndex(List_t* list, int n);

/* 
 * Free all nodes from the linkedList
 *
 * @param list pointer to the linkedList struct
 */
void deleteList(List_t* list);



#endif