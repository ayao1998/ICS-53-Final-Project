#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedList.h"
#include "protocol.h"


typedef struct {
	char * uname;
	int filedes;
} User;
//consider reader/writer locking model

typedef struct {
	char * creator;
	int creator_fd;
	char * roomname;
	struct List_t* people;
} Room;//consider reader/writer locking model


typedef struct {
	char * jobname; //msg/job/
	// some job  FIFO list thingy here
} JobBuffer;//producer/consumer access  model

typedef struct {
	int filedes;
	petr_header* petr_info;
	char* message_body;
} JobInfo;


#endif
