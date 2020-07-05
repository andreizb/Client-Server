#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <math.h>
#include "helpers.h"

void *handle_message(char *buffer, message *msg) {
	memcpy(msg->topic_name, buffer, TOPIC_NAME);
	memcpy(&(msg->type), buffer + TOPIC_NAME, 1);
	
	char message_sent[2000];
	// Se verifica formatul primit si se construiesc mesajele corespunzatoare.
	if (msg->type == INT) {
		unsigned char sign;
		memcpy(&sign, buffer + TOPIC_NAME + 1, 1);
		uint32_t nr;
		memcpy(&nr, (void *)(buffer + (TOPIC_NAME + 2)), sizeof(uint32_t));
		nr = ntohl(nr);
		if (sign == 0) {
			sprintf(msg->data, "%u", nr);
		} else if (sign == 1) {
			msg->data[0] = '-';
			sprintf(msg->data + 1, "%u", nr);
		}

		sprintf(message_sent, "%s:%d - %s - %s - %s", msg->ip,
			msg->port, msg->topic_name, "INT", msg->data);

		int size = strlen(message_sent);
		void *final_msg = calloc(1, sizeof(int) + size);
		memcpy(final_msg, &size, sizeof(int));
		memcpy(final_msg + sizeof(int), message_sent, size);

		return final_msg;
	} else if (msg->type == SHORT_REAL) {
		uint16_t nr;
		memcpy(&nr, (void *)(buffer + (TOPIC_NAME + 1)), sizeof(uint16_t));
		nr = ntohs(nr);
		sprintf(msg->data, "%.2f", (float)nr / 100);

		sprintf(message_sent, "%s:%d - %s - %s - %s", msg->ip,
			msg->port, msg->topic_name, "SHORT_REAL", msg->data);
		int size = strlen(message_sent);
		void *final_msg = calloc(1, sizeof(int) + size);
		memcpy(final_msg, &size, sizeof(int));
		memcpy(final_msg + sizeof(int), message_sent, size);

		return final_msg;
	} else if (msg->type == FLOAT) {
		unsigned char sign;
		memcpy(&sign, buffer + TOPIC_NAME + 1, 1);
		uint32_t nr;
		memcpy(&nr, (void *)(buffer + (TOPIC_NAME + 2)), sizeof(uint32_t));
		nr = ntohl(nr);
		uint8_t exp;
		memcpy(&exp, (void *)(buffer + (TOPIC_NAME + 2 + sizeof(uint32_t))),
															sizeof(uint8_t));
		if (sign == 0) {
			sprintf(msg->data, "%lf", nr * pow(10, -exp));
		} else if (sign == 1) {
			msg->data[0] = '-';
			sprintf(msg->data + 1, "%lf", nr * pow(10, -exp));
		}

		sprintf(message_sent, "%s:%d - %s - %s - %s", msg->ip,
			msg->port, msg->topic_name, "FLOAT", msg->data);
		int size = strlen(message_sent);
		void *final_msg = calloc(1, sizeof(int) + size);
		memcpy(final_msg, &size, sizeof(int));
		memcpy(final_msg + sizeof(int), message_sent, size);

		return final_msg;
	} else if (msg->type == STRING) {
		strcpy(msg->data, buffer + (TOPIC_NAME + 1));

		sprintf(message_sent, "%s:%d - %s - %s - %s", msg->ip,
			msg->port, msg->topic_name, "STRING", msg->data);
		int size = strlen(message_sent);
		void *final_msg = calloc(1, sizeof(int) + size);
		memcpy(final_msg, &size, sizeof(int));
		memcpy(final_msg + sizeof(int), message_sent, size);

		return final_msg;
	}

	// daca se ajunge la aceasta linie, avem un mesaj invalid
	strcpy(buffer, "invalid message received\n");

	return NULL;
}

int main(int argc, char *argv[])
{
	int sockfd_tcp, newsockfd, portno, sockfd_udp, flag = 1;
	char buffer[BUFLEN];
	struct sockaddr_in tcp_serv_addr, cli_addr, udp_serv_addr;
	int n, i, ret, size = 0, capacity = 1000;
	socklen_t clilen;
	user_info *users = malloc(1000 * sizeof(user_info));

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	DIE(argc < 2);

	/**
	  * Se goleste multimea de descriptori de citire (read_fds) si multimea
	  * temporara (tmp_fds).
	  */
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0);

	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0);

	portno = atoi(argv[1]);
	DIE(portno == 0);

	memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
	tcp_serv_addr.sin_family = AF_INET;
	tcp_serv_addr.sin_port = htons(portno);
	tcp_serv_addr.sin_addr.s_addr = INADDR_ANY;

	memset((char *)&udp_serv_addr, 0, sizeof(udp_serv_addr));
	udp_serv_addr.sin_family = AF_INET;
	udp_serv_addr.sin_port = htons(portno);
	udp_serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd_tcp, (struct sockaddr *)&tcp_serv_addr,
													sizeof(struct sockaddr));
	DIE(ret < 0);

	ret = bind(sockfd_udp, (struct sockaddr *)&udp_serv_addr,
													sizeof(struct sockaddr));
	DIE(ret < 0);

	ret = listen(sockfd_tcp, SOMAXCONN);
	DIE(ret < 0);

	// se realizeaza conexiunea cu stdin
	FD_SET(0, &read_fds);
	FD_SET(sockfd_udp, &read_fds);

	/** 
	  * Se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	  * in multimea read_fds
	  */
	FD_SET(sockfd_tcp, &read_fds);
	fdmax = sockfd_udp;

	while (1) {
		tmp_fds = read_fds;
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0);

		for (i = 0; i <= fdmax; i++) {
			memset(buffer, 0, BUFLEN);
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp, (struct sockaddr *)
															&cli_addr, &clilen);
					DIE(newsockfd < 0);

					// dezactivam algoritmul lui Neagle 
					n = setsockopt(newsockfd, SOL_TCP, TCP_NODELAY,
												(void *)&flag, sizeof(int));

					/** 
					  * Se adauga noul socket intors de accept() la multimea
					  * descriptorilor de citire
					  */
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					
					if (size == capacity) {
						user_info *a = realloc(users, 2 * capacity
														* sizeof(user_info));
						if (!a) {
							goto ending;
						}
						capacity *= 2;
						users = a;
					}

					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0);

					/**
					  * Daca s-a primit mesajul de conectare, cu id-ul, se
					  * va verifica dupa cum urmeaza: daca utilizatorul este
					  * deja conectat, cererea va fi respinsa, iar catre
					  * acest subscriber se va trimite "exit". Daca acesta
					  * exista dar este deconectat, vom actualiza informatiile
					  * din lista de utilizatori corespunzator si ii vom trimite
					  * toate mesajele de la topicurile la care este abonat,
					  * cu update-uri cat timp este offline, daca are astfel
					  * de topicuri si daca s-au primit mesaje.
					  * Daca nu exista, il adaugam pur si simplu.
					  */
					if (!strncmp(buffer, "connection", strlen("connection"))) {
						int j;
						for (j = 0; j < size; j++) {
							if (!strcmp(users[j].id,
											buffer + strlen("connection "))) {
								if (users[j].is_connected) {
									j = size + 1;
									break;
								} else {
									users[j].ip_struct = cli_addr.sin_addr;
									users[j].port = ntohs(cli_addr.sin_port);
									users[j].socket = newsockfd;
									users[j].is_connected = 1;
									printf("New client ");
									printf("%s connected from %s:%d.\n",
										users[j].id, inet_ntoa(users[j]
												.ip_struct), users[j].port);

									while (users[j].q->front != NULL) {
										int x;
										memcpy(&x, users[j].q->front->key, sizeof(int));
										send(users[j].socket, users[j].q->front->key, x + sizeof(int), 0);
										deQueue(users[j].q);
									}

									j = size + 2;
									break;
								}
							}	
						}

						if (j == size + 1) {
							strcpy(buffer, "exit\n");
							n = send(newsockfd,
											(const void *)buffer, BUFLEN, 0);
							DIE(n < 0);

							FD_CLR(newsockfd, &read_fds);
							continue;
						} else if (j == size + 2) {
							continue;
						}

						strcpy(users[size].id, buffer + strlen("connection "));

						users[size].ip_struct = cli_addr.sin_addr;
						users[size].port = ntohs(cli_addr.sin_port);
						users[size].socket = newsockfd;
						users[size].is_connected = 1;
						// Mesaj connection
						printf("New client %s connected from %s:%d.\n",
							users[size].id, inet_ntoa(users[size].ip_struct)
															, users[size].port);
						users[size].topics_number = 0;
						users[size].topics_capacity = 1000;
						users[size].subscribed_topics = calloc(1000,
															sizeof(topic *));
						users[size].q = createQueue();
						size++;
					}
				} else if (i == sockfd_udp) {
					printf("Da\n");
					socklen_t sizeOfStruct = sizeof(udp_serv_addr);
					memset(buffer, 0, BUFLEN);
					n = recvfrom(sockfd_udp, buffer, BUFLEN, 0,
							((struct sockaddr*)&udp_serv_addr), &sizeOfStruct);
					DIE(n < 0);

					message *msg = calloc(1, sizeof(message));
					if (!msg) {
						goto ending;
					}

					sprintf(msg->ip, "%s", inet_ntoa(udp_serv_addr.sin_addr));
					msg->port = ntohs(udp_serv_addr.sin_port);
					void *message_computed = handle_message(buffer, msg);
					int x;
					memcpy(&x, message_computed, sizeof(int));
					/**
					  * Daca mesajul primit este valid (verificarea se face in
					  * functia apelata anterior), putem continua cu trimiterea
					  * acestui mesaj catre toti clientii abonati.
					  * In cazul unui mesaj invalid, eliberam memoria si nu
					  * trimitem mesajul, lasand serverul sa isi continue
					  * executia normal.
					  */
					if (strcmp(buffer, "invalid message received\n")) {
						for (int j = 0; j < size; j++) {
							for (int t = 0; t < users[j].topics_number; t++) {
								if (!strcmp(users[j].subscribed_topics[t]
											->topic_name, msg->topic_name)) {
									if (users[j].is_connected == 1) {
										n = send(users[j].socket,
												message_computed, x + sizeof(int), 0);
										DIE(n < 0);
									} else if (users[j].subscribed_topics[t]
															->recv_prev == 1) {
										enQueue(users[j].q, message_computed);
									}
									break;
								}
							}
						}
					} else {
						free(msg);
					}
				} else if (i == 0) {
					fgets(buffer, BUFLEN, stdin);
					/** 
					  * Comenzile diferite de "exit" de la stdin vor fi
					  *	ignorate.
					  */
					if (!strcmp(buffer, "exit\n") || !strlen(buffer)) {
						/**
						  * Vom folosi "goto" pentru a iesi simplu din cele
						  * doua structuri repetitive. Eliberarea memoriei
						  * si notificarea subscriberilor se va realiza la
						  * eticheta indicata, la sfarsitul programului.
						  */
						goto ending;
					}
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0);

					if (n == 0) {
						// conexiunea s-a inchis
						for (int j = 0; j < size; j++) {
							/**
							  * Din cauza ca este posibil ca mai multe conturi
							  * sa fi fost create cu acelasi "socket", vom
							  * da disconnect unui utilizator care este neaparat
							  * conectat.
							  */
							if (i == users[j].socket && users[j].is_connected) {
								printf("Client %s disconnected.\n",
																users[j].id);
								users[j].is_connected = 0;
								close(i);
								break;
							}
						}

						FD_CLR(i, &read_fds);
					} else {
						if (!strncmp(buffer, "subscribe",
														strlen("subscribe"))) {
							/**
							  * Implementarea subscribe, cu tratari preventive.
							  * Daca comanda nu este exact "subscribe" SAU
							  * topicul are o lungime mai mare decat maximul
							  * posibil vom continua executia serverului, 
							  * ignorand aceasta comanda.
							  * Daca totul este in regula, adaugam topicul in
							  * lista topicurilor urmarite de utilizator.
							  */
							char *p = strtok(buffer, " \n");
							if (strcmp(p, "subscribe")) {
								continue;
							}

							p = strtok(NULL, " \n");
							if (strlen(p) > TOPIC_NAME) {
								continue;
							}

							/**
							  * Se verifica daca topicul exista deja in lista de
							  * topicuri a utilizatorului. Daca exista, nu se
							  * va adauga nimic. In caz contrar, se va adauga
							  * acest topic, facandu-se realocarea listei daca
							  * este cazul. De asemenea, daca topicul are SF 1,
							  * se verifica daca utilizatorul detine o lista
							  * in care se pot salva mesajele. Daca nu, se va
							  * aloca una.
							  */
							char *topic_name = p;
							p = strtok(NULL, " \n");
							for (int j = 0; j < size; j++) {
								if (i == users[j].socket
													&& users[j].is_connected) {
									for (int t = 0; t <
											users[j].topics_number; t++) {
										if (!strcmp(topic_name, users[j].
													subscribed_topics[t]->
													topic_name)) {
											j = size;
											break;
										}
									}

									if (j == size) {
										break;
									}

									if (users[j].topics_number ==
												users[j].topics_capacity) {
										topic **a =
										realloc(users[j].subscribed_topics,
											2 * users[j].topics_number
											* sizeof(topic *));
										if (!a) {
											goto ending;
										}

										users[j].topics_capacity *= 2;
										users[j].subscribed_topics = a;
									}
									int k = users[j].topics_number;
									users[j].topics_number++;
									users[j].subscribed_topics[k] =
												calloc(1, sizeof(topic));
									topic *t
										= users[j].subscribed_topics[k];
									if (!t) {
										goto ending;
									}

									strcpy(t->topic_name, topic_name);
									t->recv_prev = atoi(p);
									break;
								}
							}
						} else if (!strncmp(buffer, "unsubscribe",
													strlen("unsubscribe"))) {
							/**
							  * Implementarea unsubscribe. Se cauta topicul
							  * primit si daca este gasit in lista de topicuri
							  * a utilizatorului, acest topic este sters.
							  */
							char *p = strtok(buffer, " \n");
							if (strcmp(p, "unsubscribe")) {
								continue;
							}

							p = strtok(NULL, " \n");
							if (strlen(p) > TOPIC_NAME) {
								continue;
							}

							for (int j = 0; j < size; j++) {
								if (i == users[j].socket
													&& users[j].is_connected) {
									for (int t = 0; t < users[j].topics_number;
																		t++) {
										if (!strcmp(p,
												users[j].subscribed_topics[t]->
																topic_name)) {
											free(users[j].subscribed_topics[t]);
											for (int k = t + 1; k <
												users[j].topics_number; k++) {
												users[k - 1] = users[k];
											}
											users[j].topics_number--;
											j = size;
											break;
										}
									}
								}
							}
						} else {
							/**
							  * Nu ar trebui sa primim nimic altceva de la
							  * subscriberi. Daca primim totusi ceva, prin
							  * absurd, vom ignora pur si simplu.
							  */
							continue;
						}
					}
				}
			}
		}
	}

	// Se inchid toti socketii deschisi, iar memoria este eliberata.
	ending:
	for (i = 0; i < size; i++) {
		for (int t = 0; t < users[i].topics_capacity; t++) {
			free(users[i].subscribed_topics[t]);
		}
		free(users[i].subscribed_topics);

		if (users[i].is_connected == 1) {
			n = send(users[i].socket, (const void *)buffer, BUFLEN, 0);
			DIE(n < 0);
			close(users[i].socket);
		}

	}
	free(users);

	close(sockfd_tcp);
	close(sockfd_udp);

	return 0;
}
