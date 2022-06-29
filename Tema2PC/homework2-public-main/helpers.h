#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

using namespace std;

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define MAX_TCP      50
#define UDP_BUFLEN   1551 
#define UDP_TO_CLIENT_BUFLEN 1585
#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

struct udpMsg {
	char topic[50];
	unsigned char type;
	char content[1500];
} __attribute__((packed));

struct udp_to_client {
	char udp_cli_ip[30];
	int udp_cli_port;
	struct udpMsg udpMsg;
} __attribute__((packed));

typedef struct tcp {
	char id[30];
	int sockFd;
	vector <string> subscriptions;
} tcpClient;

typedef struct tcpmsg {
	char subscribe_type[10];
	char topic[50];
} tcpMessage;

#endif
