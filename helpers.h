#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#define DIE(assertion)	\
	do {									\
		if (assertion) {					\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN 		 1551	// dimensiunea maxima a calupului de date
#define TOPIC_NAME   50
#define CONTENT_SIZE 1500
#define MESSAGE_SIZE 1572

#define INT 		 0
#define SHORT_REAL 	 1
#define FLOAT		 2
#define STRING       3

typedef struct {
	char ip[16];
	int port;
	char topic_name[TOPIC_NAME];
	unsigned char type;
	char data[CONTENT_SIZE];
} message;

typedef struct {
	char topic_name[TOPIC_NAME];
	unsigned char recv_prev;
} topic;

typedef struct {
	char id[10];
	int port;
	int socket;
	struct in_addr ip_struct;
	int is_connected;
	topic **subscribed_topics;
	int topics_number;
	int topics_capacity;
	struct Queue *q; 
} user_info;

#endif
