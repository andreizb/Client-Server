#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "helpers.h"
#include <math.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s subscriber_id server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, fdmax, flag = 1;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	fd_set read_fds, temp_fds;
	FD_ZERO(&temp_fds);
	FD_ZERO(&read_fds);

	if (argc < 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0);

	// dezactivam algoritmul lui Neagle
	n = setsockopt(sockfd, SOL_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));
	DIE(n < 0);

	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	FD_SET(0, &read_fds);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0);

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0);

	memset(buffer, 0, BUFLEN);
	char msg[100] = "connection ";
	strcat(msg, argv[1]);
	memcpy(buffer, msg, sizeof(msg));
	send(sockfd, buffer, sizeof(buffer), 0);

	while (1) {
		temp_fds = read_fds;

		ret = select(fdmax + 1, &temp_fds, NULL, NULL, NULL);
		DIE(ret < 0);

		memset(buffer, 0, BUFLEN);
		if (FD_ISSET(0, &temp_fds)) {
  			// s-a primit o comanda de la tastatura
			fgets(buffer, BUFLEN - 1, stdin);

			if (!strncmp(buffer, "subscribe", 9)) {
				char aux[BUFLEN];
				strcpy(aux, buffer);
				char *p = strtok(buffer, " \n");
				if (strcmp(p, "subscribe")) {
					continue;
				}

				p = strtok(NULL, " \n");
				if (strlen(p) > TOPIC_NAME) {
					continue;
				}
				p = strtok(NULL, " \n");

				if (!p || (strcmp(p, "1") && strcmp(p, "0"))) {
					continue;
				}

				// se trimite mesaj la server
				n = send(sockfd, aux, strlen(aux), 0);
				DIE(n < 0);

				char msg[2000];
				strcpy(msg, "subscribed ");
				strncat(msg, buffer + 10, strlen(buffer) - 12);
				printf("%s\n", msg);
			} else if (!strncmp(buffer, "unsubscribe", 11)) {
				char aux[BUFLEN];
				strcpy(aux, buffer);
				char *p = strtok(buffer, " \n");
				if (strcmp(p, "unsubscribe")) {
					continue;
				}

				p = strtok(NULL, " \n");
				if (strlen(p) > TOPIC_NAME) {
					continue;
				}
				p = strtok(NULL, " \n");

				if (p) {
					continue;
				}

				// se trimite mesaj la server
				n = send(sockfd, aux, strlen(aux), 0);
				DIE(n < 0);

				char msg[2000];
				strcpy(msg, "unsubscribed ");
				strncat(msg, buffer + 12, strlen(buffer) - 13);
				printf("%s\n", msg);
			} else if (!strcmp(buffer, "exit\n")) {
				// se trimite mesaj la server
				n = send(sockfd, buffer, strlen(buffer), 0);
				DIE(n < 0);

				break;
			} else {
				/**
				  * Comenzile invalide sunt ignorate, deci nu se vor trimite.
				  * Nu se va afisa niciun mesaj de eroare, conform cerintei.
				  */
				continue;
			}
		} else {
			// s-a primit un mesaj de la server, "exit" sau update la un topic
			// oprim la "n <= 0" pentru cazul in care serverul se inchide brusc
			n = recv(sockfd, buffer, sizeof(int), 0);
			if (n <= 0) {
				break;
			}

			if (!strncmp(buffer, "exit", 4)) {
				break;
			} else {
				int size;
				memcpy(&size, buffer, sizeof(int));

				n = recv(sockfd, buffer, size, 0);
				if (n <= 0) {
					break;
				}

				printf("%s\n", buffer);
			}
		}
	}

	close(sockfd);
	return 0;
}
