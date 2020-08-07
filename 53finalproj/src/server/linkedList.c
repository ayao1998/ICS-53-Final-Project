#include "linkedList.h"
/*
    What is a linked list?
    A linked list is a set of dynamically allocated nodes, arranged in
    such a way that each node contains one value and one pointer.
    The pointer always points to the next member of the list.
    If the pointer is NULL, then it is the last node in the list.

    A linked list is held using a local pointer variable which
    points to the first item of the list. If that pointer is also NULL,
    then the list is considered to be empty.
    -----------------------------              -------------------------------
    |              |             |            \ |              |             |
    |     DATA     |     NEXT    |--------------|     DATA     |     NEXT    |
    |              |             |            / |              |             |
    ------------------------------              ------------------------------
*/
void printUserList(List_t* listHead)  {
	node_t * node = listHead->head;
    while(node) {
       //printf("name is : %s\n", ((User*)(node->value))->uname);
       node = node->next;
    }
  
    return;
}

void printJobList(List_t* listHead)  {
	node_t * node = listHead->head;
    while(node) {
      // printf("value: %d\n", ((JobInfo*)(node->value))->petr_info->msg_type);
       node = node->next;
    }
  
    return;
}

int findbyroom(List_t* listHead, char * roomname) {
	node_t * node;
	node = listHead->head;
    while(node) {
   // printf("Username is : %s\n", username);
	//printf("Username is being compared to uname %s\n", ((User *)(node->value))->uname);
        if(strcmp(((Room *)(node->value))->roomname, roomname) == 0)
	    {
            	return -1;
	    }
        node = node->next;
    }
    return 0;
}

int findroomindex(List_t* listHead, char * roomname) {
	node_t * node;
	node = listHead->head;
	int counter = 0;
    while(node) {
   // printf("Username is : %s\n", username);
	//printf("Username is being compared to uname %s\n", ((User *)(node->value))->uname);
        if(strcmp(((Room *)(node->value))->roomname, roomname) == 0)
	    {
            	return counter;
	    }
        node = node->next;
	counter++;
    }
    return -1; //in the case it is not found
}


int findbyname(List_t* listHead, char * username) {
	node_t * node;
	node = listHead->head;
    while(node) {
   // printf("Username is : %s\n", username);
	//printf("Username is being compared to uname %s\n", ((User *)(node->value))->uname);
        if(strcmp(((User *)(node->value))->uname, username) == 0)
	{
            return -1;
	}
        node = node->next;
    }
    return 0;
}

void insertFront(List_t* list, void* valref) {
    if (list->length == 0)
        list->head = NULL;

    node_t** head = &(list->head);
    node_t* new_node;
    new_node = malloc(sizeof(node_t));

    new_node->value = valref;

    new_node->next = *head;
    *head = new_node;
    list->length++; 
}

void insertRear(List_t* list, void* valref) {
    if (list->length == 0) {
        insertFront(list, valref);
        return;
    }

    node_t* head = list->head;
    node_t* current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = malloc(sizeof(node_t));
    current->next->value = valref;
    current->next->next = NULL;
    list->length++;
}

void insertInOrder(List_t* list, void* valref) {
    if (list->length == 0) {
        insertFront(list, valref);
        return;
    }

    node_t** head = &(list->head);
    node_t* new_node;
    new_node = malloc(sizeof(node_t));
    new_node->value = valref;
    new_node->next = NULL;

    if (list->comparator(new_node->value, (*head)->value) <= 0) {
        new_node->next = *head;
        *head = new_node;
    } 
    else if ((*head)->next == NULL){ 
        (*head)->next = new_node;
    }                                
    else {
        node_t* prev = *head;
        node_t* current = prev->next;
        while (current != NULL) {
            if (list->comparator(new_node->value, current->value) > 0) {
                if (current->next != NULL) {
                    prev = current;
                    current = current->next;
                } else {
                    current->next = new_node;
                    break;
                }
            } else {
                prev->next = new_node;
                new_node->next = current;
                break;
            }
        }
    }
    list->length++;
}

void* removeFront(List_t* list) {
    node_t** head = &(list->head);
    void* retval = NULL;
    node_t* next_node = NULL;

    if (list->length == 0) {
        return NULL;
    }

    next_node = (*head)->next;
    retval = (*head)->value;
    list->length--;

    node_t* temp = *head;
    *head = next_node;
    free(temp);

    return retval;
}

void* removeRear(List_t* list) {
    if (list->length == 0) {
        return NULL;
    } else if (list->length == 1) {
        return removeFront(list);
    }

    void* retval = NULL;
    node_t* head = list->head;
    node_t* current = head;

    while (current->next->next != NULL) { 
        current = current->next;
    }

    retval = current->next->value;
    free(current->next);
    current->next = NULL;

    list->length--;

    return retval;
}

/* indexed by 0 */
void* removeByIndex(List_t* list, int index) {
    if (list->length <= index) {
        return NULL;
    }

    node_t** head = &(list->head);
    void* retval = NULL;
    node_t* current = *head;
    node_t* prev = NULL;
    int i = 0;

    if (index == 0) {
        retval = (*head)->value;
        
		node_t* temp = *head;
        *head = current->next;
        free(temp);
        
		list->length--;
        return retval;
    }

    while (i++ != index) {
        prev = current;
        current = current->next;
    }

    prev->next = current->next;
    retval = current->value;
    free(current);

    list->length--;

    return retval;
}

void deleteList(List_t* list) {
    if (list->length == 0)
        return;
    while (list->head != NULL){
        removeFront(list);
    }
    list->length = 0;
}

void sortList(List_t* list) {
    List_t* new_list = malloc(sizeof(List_t));	
    
	new_list->length = 0;
    new_list->comparator = list->comparator;
    new_list->head = NULL;

    int i = 0;
    int len = list->length;
    for (; i < len; ++i)
    {
        void* val = removeRear(list);
        insertInOrder(new_list, val); 
    }

    node_t* temp = list->head;
    list->head = new_list->head;

    new_list->head = temp;
    list->length = new_list->length;  

    deleteList(new_list);
    free(new_list);  
}