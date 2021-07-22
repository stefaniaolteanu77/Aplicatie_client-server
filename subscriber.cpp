#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <bits/stdc++.h>
#include "utils.h"


int main(int argc, char *argv[])
{	
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	int flag = 1;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 4) {
		usage_subscriber(argv[0]);
	}

	// creez socket-ul pentru conexiunea cu serverul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	//completez campurile din serv.addr
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	// creez conexiunea cu serverul
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	// setez file descriptorii
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);


	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd,  &read_fds);
	fdmax = sockfd;
	
	// dezactivez algoritmul lui Nagle
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

	memset(buffer, 0, BUFLEN);
	char client_id[10];
	memcpy(client_id, argv[1], 10);
	n = send(sockfd, client_id, sizeof(client_id), 0);
	DIE(n < 0, "Send");

	struct request request;

	while (1) {
  		
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			buffer[strlen(buffer) - 1] = '\0';
			
			// citesc de la tastatura comanda de exit
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			} else {
				// citesc de la tastatura si trimit comanda de
				// subscribe/unsubscribe catre server
				parse_command(buffer, &request);
				n = send(sockfd, (char*)&request, sizeof(request), 0);
				DIE(n < 0, "Couldn't send request to server");

				if (strcmp(request.command, "subscribe") == 0) {
					printf("Subscribed to topic.\n");
				} else {
					printf("Unsubscribed from topic.\n");
				}
			}

		} else if (FD_ISSET(sockfd, &tmp_fds)){
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, BUFLEN, 0);
			
			// daca nu am primit nimic sau primesc "disconnect"
			// se inchide clientul 
			if (n == 0 || strcmp(buffer, "disconnect") == 0) {
				break;
			}
			DIE(n < 0, "recv");
			// printez mesajul de la server
			printf("%s\n", buffer);
		}		
	}

	fflush(stdout);
	close(sockfd);

	return 0;
}
