#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, ret1;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char udp_buffer[UDP_TO_CLIENT_BUFLEN];
	fd_set read_fds, tmp_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	if (argc < 3) {
		usage(argv[0]);
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// seteaza valoarea indexului maxima
	int fdmax;
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	if (STDIN_FILENO < sockfd)
		fdmax = sockfd;
	else
		fdmax = STDIN_FILENO;

	// Trimite id-ul catre server
	send(sockfd, argv[1], 20, 0);

	while (1) {
		tmp_fds = read_fds;
		
		ret1 = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret1 < 0, "Failed to select");

		if (FD_ISSET(0, &tmp_fds)){ // se citeste de la stdin

			memset(buffer, 0, sizeof(buffer));
			n = read(0, buffer, sizeof(buffer) - 1);
			DIE(n < 0, "read");

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}
			if(strncmp(buffer,"subscribe", 9) == 0) {
				printf("Subscribed to topic.\n");
				n = send(sockfd, buffer, strlen(buffer), 0);
			}
			if(strncmp(buffer,"unsubscribe",11) == 0) {
				printf("Unsubscribed to topic.\n");
				n = send(sockfd, buffer, strlen(buffer), 0);
			}

			DIE(n < 0, "send");
		} 
		else if (FD_ISSET(sockfd, &tmp_fds)) { // se primesc mesaje de la udp
			memset(udp_buffer, 0, sizeof(buffer));
			socklen_t server_len = sizeof(serv_addr);

			n = recvfrom(sockfd, udp_buffer, UDP_TO_CLIENT_BUFLEN, 0, (struct sockaddr *)&serv_addr, &server_len);

			if (n == 0) {
				break;
			}
			struct udp_to_client *recv_msg = (struct udp_to_client *) udp_buffer;
			
			switch (recv_msg->udpMsg.type)
			{
			case 0:{ // integer
				int number;
				unsigned char sign = (unsigned char)recv_msg->udpMsg.content[0];
				number = ntohl(*(int*)(recv_msg->udpMsg.content + 1));
				if(sign == 1) {
					printf("%s:%d - %s - INT - -%u\n", recv_msg->udp_cli_ip, recv_msg->udp_cli_port, recv_msg->udpMsg.topic, number);
				}else
				{
					printf("%s:%d - %s - INT - %u\n", recv_msg->udp_cli_ip, recv_msg->udp_cli_port, recv_msg->udpMsg.topic, number);
				}				
				break;
			}
			case 1:{ // short
				uint16_t number = ntohs(*(uint16_t*)(recv_msg->udpMsg.content));
				
				printf("%s:%d - %s - SHORT_REAL - %.2f\n", recv_msg->udp_cli_ip, recv_msg->udp_cli_port, recv_msg->udpMsg.topic, (float)number/100);
				break;
			}
			case 2:{ // float
				uint32_t number;
				uint32_t p = 1;
				unsigned char sign = (unsigned char)recv_msg->udpMsg.content[0];
				unsigned char exp = (unsigned char)recv_msg->udpMsg.content[5];
				number = ntohl(*(int*)(recv_msg->udpMsg.content + 1));
				for(int j = 0; j < exp; j++){
					p = p*10;
				}
				if(sign == 1){
					printf("%s:%d - %s - FLOAT - -%.4f\n", recv_msg->udp_cli_ip, recv_msg->udp_cli_port, recv_msg->udpMsg.topic, (float)number/(float)p);
				}else
				{
					printf("%s:%d - %s - FLOAT - %.4f\n", recv_msg->udp_cli_ip, recv_msg->udp_cli_port, recv_msg->udpMsg.topic, (float)number/(float)p);
				}
				
				break;
			}
			case 3:{ // string
				recv_msg->udpMsg.content[1500] = '\0';
				printf("%s:%d - %s - STRING - %s\n", recv_msg->udp_cli_ip, recv_msg->udp_cli_port, recv_msg->udpMsg.topic, recv_msg->udpMsg.content);
				break;
			}
			default:
				break;
			}
		}
	}

	close(sockfd);

	return 0;
}
