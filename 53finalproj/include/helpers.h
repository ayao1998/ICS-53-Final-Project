#ifndef HELPERS_H
#define HELPERS_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "linkedList.h"
#include "structs.h"
#include <pthread.h>

void exit_chat_rooms(int fileds, JobInfo* job_info, List_t* rooms, List_t* users, pthread_mutex_t* lock);
#endif