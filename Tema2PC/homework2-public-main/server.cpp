#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int  sockfd_TCP, sockfd_UDP, newsockfd, portno;
	char buffer[BUFLEN];
	char udp_buffer[UDP_BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd_TCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_TCP < 0, "socketTCP");

	sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_UDP < 0, "socketUDP");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd_TCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bindTCP");

	bind(sockfd_UDP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bindUDP");

	ret = listen(sockfd_TCP, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd_TCP, &read_fds);
	FD_SET(sockfd_UDP,&read_fds);

	// seteaza valoarea socket-ului maxima
	if(sockfd_UDP < sockfd_TCP)
		fdmax = sockfd_TCP;
	else
		fdmax = sockfd_UDP;
	
	// dezactivarea algoritmului lui Nagle
	int flag = 1;
	ret = setsockopt(sockfd_TCP, IPPROTO_TCP, 1, (void *)&flag, sizeof(flag));
	DIE(ret < 0, "setsockopt");

	tcpClient subscribers[MAX_TCP];
	int size_tcp = 0;

	for (int i = 0; i < MAX_TCP; i++) {
		subscribers[i].sockFd = -1;
	}

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_TCP) {
					// a venit o cerere de conexiune pe socketul tcp inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_TCP, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					char id[20];
					recv(newsockfd, id, 20, 0);

					// se cauta daca clientul este deja conenctat
					// daca da, se sterge socket-ul intors de accept si se trece mai departe
					int ok = 1;
					for (int j = 0; j < size_tcp; j++) {
						if (strcmp(id, subscribers[j].id) == 0) {
							if (subscribers[j].sockFd != -1) {
								printf("Client %s already connected.\n", id);
								close(newsockfd);
								FD_CLR(newsockfd,&read_fds);
								ok = 0;
								break;
							}else
							{
								subscribers[j].sockFd = newsockfd;
								printf("New client %s connected from %s:%u.\n", id, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
								ok = 0;
								break;
							}
							
						}
					}
					if (ok){ // daca nu exista deja id-ul clientului:
						subscribers[size_tcp].sockFd = newsockfd;
						strcpy(subscribers[size_tcp].id, id);
						size_tcp++;
						printf("New client %s connected from %s:%u.\n", id, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
						break;
					}


				} else if (i == sockfd_UDP) {
					memset(udp_buffer, 0, UDP_BUFLEN);
					clilen = sizeof(cli_addr);
					int bytes = recvfrom(sockfd_UDP, udp_buffer, UDP_BUFLEN, 0, (struct sockaddr *)&cli_addr, &clilen);
					DIE(bytes < 0, "Failed to receive UDP message.");

					//primeste ip-ul si portul pentru clientul udp
					char *udp_cli_ip = inet_ntoa(cli_addr.sin_addr);
					int udp_cli_port = ntohs(cli_addr.sin_port);

					// parseaza mesajul udp in udp_to_client pentru a fi trimis 
					struct udpMsg *udp_msg = (struct udpMsg *)udp_buffer;

					// copiaza campurile de adresa si de mesaj in structura de mai jos
					struct udp_to_client message;
					memset(message.udp_cli_ip, 0, sizeof(message.udp_cli_ip));
					strncpy(message.udp_cli_ip, udp_cli_ip, strlen(udp_cli_ip));
					message.udp_cli_port = udp_cli_port;

					memset(message.udpMsg.topic, 0, sizeof(message.udpMsg.topic));
					strncpy(message.udpMsg.topic,udp_msg->topic,strlen(udp_msg->topic));
					message.udpMsg.type = udp_msg->type;
					memset(message.udpMsg.content,0,sizeof(message.udpMsg.content));
					memcpy(message.udpMsg.content,udp_msg->content,sizeof(udp_msg->content));

					// trimite pachetul catre clientii abonati si care sunt online
					for (int j = 0; j < size_tcp; j++) {
						if(subscribers[j].sockFd != -1) {
							if(find(subscribers[j].subscriptions.begin(),subscribers[j].subscriptions.end(),udp_msg->topic) != subscribers[j].subscriptions.end()) {
								send(subscribers[j].sockFd,(char *)&message,sizeof(struct udp_to_client),0);
							}
						}
					}
										
				}
				else if (i == STDIN_FILENO) { // se citeste de la stdin
			 		memset(buffer, 0, BUFLEN);
			 		read(0,buffer,BUFLEN);

					// serverul se inchide
			 		if (strncmp(buffer, "exit", 4) == 0) {
						for(int j = 0; j < size_tcp; j++)
							if (subscribers[j].sockFd != -1) {
								send(subscribers[j].sockFd, NULL, 0, 0);
								FD_CLR(subscribers[j].sockFd, &read_fds);
								close(subscribers[j].sockFd);
							}
						close(sockfd_TCP);
						close(sockfd_UDP);
			 			return 0;
			 		}

				} else {
					// s-au primit date pe unul din socketii de client tcp
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer) - 1, 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						for (int j = 0; j < size_tcp; j++) {
							if(i == subscribers[j].sockFd) {
								printf("Client %s disconnected.\n", subscribers[j].id);
								
								// se scoate din multimea de citire socketul inchis 
								FD_CLR(i, &read_fds);
								subscribers[j].sockFd = -1;
								close(i);

								break;
							}
						}
					} else {
						char* subscribe_type = strtok(buffer," \n"); // subscribe sau unsubscribe
						char* topic = strtok(NULL," \n");   // topicul la care se doreste abonarea/dezabonarea
						if(topic[strlen(topic) - 1] == '\n'){
               				topic[strlen(topic) - 1] = '\0'; // eliminarea new-line-ului
            			}
						if (strncmp(subscribe_type,"subscribe",9) == 0) { // pentru sunscribe
							for (int j = 0; j < size_tcp; j++) {
								if(i == subscribers[j].sockFd) {
									if (find(subscribers[j].subscriptions.begin(),subscribers[j].subscriptions.end(),topic) == subscribers[j].subscriptions.end()){
										subscribers[j].subscriptions.push_back(topic);
									}
								}
							}	
						}
						if(strncmp(subscribe_type,"unsubscribe",11) == 0) { // pentru unsubscribe
							for(int j = 0; j < size_tcp; j++) {
								if(i == subscribers[j].sockFd) {
									subscribers[j].subscriptions.erase(std::remove(subscribers[j].subscriptions.begin(), subscribers[j].subscriptions.end(), topic), subscribers[j].subscriptions.end()); 
								}
							}
						}
					}
				} 
			}
		}
	}

	close(sockfd_TCP);

	return 0;
}
