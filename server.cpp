#include <bits/stdc++.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fstream>
#include "utils.h"


using namespace std;

int main(int argc, char *argv[])
{	
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	
	// map-ul cu clientii ce contine id_ul clientului si fd
	unordered_map<string, int> clients;

	// map-ul intre id-ul clientului si un vector de mesaje in care
	// se pastreaza mesajele ce vor urma sa fie trimise 
	unordered_map<string, vector<string>> backlog_messages;

	//map intre topic si un map intre subscriber si sf
	unordered_map<string, unordered_map<subscriber, int>> online_subsc;

	//map intre id-ul clientului si map intre topic si sf
	unordered_map<string, unordered_map<string, int>> offline_subsc;

    int udp_fd, tcp_fd, new_tcp_fd, num_port;
    int ret_val, n;
	int flag = 1;
    char buffer[BUFLEN];
    struct sockaddr_in server_addr, udp_addr, client_addr;
    socklen_t client_len, udp_len;
	udp_len = sizeof(sockaddr_in);
	client_len =  sizeof(sockaddr_in);

	struct request* request;
    fd_set read_fds;
	fd_set tmp_fds;	
	int fdmax;

    if (argc < 2) {
		usage_server(argv[0]);
	}

	// cream socket UDP
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_fd < 0, "Failed to open upd socket");

	// cream socket TCP pasiv
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_fd < 0, "Failed to open tcp socket");

    num_port = atoi(argv[1]);
	DIE(num_port == 0, "atoi");

	// completez informatia din server_addr
	memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(num_port);
	server_addr.sin_addr.s_addr = INADDR_ANY;


    int enable_udp = 1;
	DIE(setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &enable_udp, sizeof(int)) == -1 , "Address already in use");
	ret_val = bind(udp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
	DIE(ret_val < 0, "bind");


    int enable_tcp = 1;
	DIE(setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &enable_tcp, sizeof(int)) == -1 , "Address already in use");
	ret_val = bind(tcp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
	DIE(ret_val < 0, "bind");


    ret_val = listen(tcp_fd, MAX_CLIENTS);
	DIE(ret_val < 0, "listen");

	// setez file descriptorii
    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);


    FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(udp_fd, &read_fds);
	FD_SET(tcp_fd, &read_fds);
	fdmax = tcp_fd;

	int close_server = 0;



    while (1) {
		tmp_fds = read_fds; 
		
		ret_val = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret_val < 0, "select");

		for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					// primesc de la tastatura comanda exit
					if (strncmp(buffer, "exit\n", 5) == 0) {
						for (int j = 5; j <= fdmax; j++) {
							FD_CLR(j, &read_fds);
							close(j);
						}
						close_server = 1;
						break;
					}

                } else if (i == udp_fd) {
                    memset(buffer, 0, BUFLEN);

					// primesc mesajul de la clientul UDP
					int received = recvfrom(udp_fd, buffer, BUFLEN, 0,  (struct sockaddr *) &(udp_addr), &udp_len);
					DIE(received < 0, "recvfrom");
                	struct udp_message* udp_message = (struct udp_message*)buffer;
                    struct tcp_message tcp_message;
					char type[11];

					// decodez mesajul de la clientul UDP
					make_server_message(udp_addr.sin_addr, udp_addr.sin_port, udp_message, &tcp_message, type);
					sprintf(buffer,"%s:%d - %s - %s - %s", inet_ntoa(tcp_message.address), ntohs(tcp_message.num_port), 
															tcp_message.topic, type, tcp_message.payload);
					
					// pentru toti clientii online care sunt abonati la topic se trimite mesajul
					for (auto& x : online_subsc[tcp_message.topic]) {
						n = send(x.first.fd, buffer, sizeof(buffer), 0);
						DIE(n < 0, "Couldn't send message");
					}

					// pentru toti clientii offline dar abonati la topic cu sf = 1 se salveaza
					// mesajul pentru a se trimite ulterior
					for (auto& x : offline_subsc) {
						for (auto& y : x.second) {
							if (y.first == tcp_message.topic && y.second == 1) {
								backlog_messages[x.first].push_back(buffer);
							}
						}
					}

                } else if (i == tcp_fd) {
                    client_len = sizeof(client_addr);
					
					// se accepta o cerere de conexiune TCP
					new_tcp_fd = accept(tcp_fd, (struct sockaddr *) &(client_addr), &client_len);
					DIE(new_tcp_fd < 0, "accept");

					// dezactivarea algoritmului Nagle 
					setsockopt(new_tcp_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
					
					// se primeste id-ul clientului
					char id_client[10];
					n = recv(new_tcp_fd, id_client, 10, 0);
					DIE(n < 0, "Didn't get client id");

					//clientul nu se afla in map-ul de clienti
					if (clients.find(id_client) == clients.end()) {
						// se aduga in map
						clients[id_client] = new_tcp_fd;

						// se cauta daca are mesaje salvate in urma abonarilor cu sf = 1
						// din conectari precedente si daca are se trimit si se sterg 
						if (backlog_messages.find(id_client) != backlog_messages.end()) {
							for (string s : backlog_messages[id_client]) {
								memset(buffer, 0 , BUFLEN);
								strcpy(buffer, s.c_str());
								n = send(new_tcp_fd, buffer , BUFLEN, 0);
								DIE(n < 0, "fail");
							}
							backlog_messages[id_client].clear();
						}
						
						// daca se afla in subscriberii offline se muta 
						// in cei online
						for(auto& x : online_subsc) {
							for(auto& y: offline_subsc[id_client]) {
								if(x.first == y.first) {
									struct subscriber sub;
									sub.fd = new_tcp_fd;
									sub.id = id_client;
									online_subsc[x.first][sub] = y.second;
								}
							}
						}
						offline_subsc[id_client].clear();


						FD_SET(new_tcp_fd, &read_fds);
						if (new_tcp_fd > fdmax) {
							fdmax = new_tcp_fd;
						}

						printf("New client %s connected from %s:%d.\n", id_client,
								inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					} else {
						//  s-a conectat un client cu acelasi id cu al altui client deja conectat
						printf("Client %s already connected.\n", id_client);

						// trimitem un mesaj special pentru a deconecta clientul
						memset(buffer, 0, BUFLEN);
						strcpy(buffer, "disconnect");
						n = send(new_tcp_fd, buffer, 11, 0);
						DIE(n < 0, "Couldn't send disconnect message");
					}

                } else {
                    memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

                    if (n == 0) {
						char id[10];

						// scoatem clientul din map-ul de clienti conectati
						for (auto& x : clients) {
							if (x.second == i) {
								strcpy(id, x.first.c_str());
								clients.erase(x.first);
								break;
							}
						}
						printf("Client %s disconnected.\n", id);
	
						// mutam clientul din map-ul de clienti online in cel de 
						// clienti offline
						for (auto& x : online_subsc) {
							for (auto& y : x.second) {
								if (y.first.id == id) {
									offline_subsc[id][x.first] = y.second;
									x.second.erase(y.first);
									break;
								}
							}				
						}
						
						close(i);
						FD_CLR(i, &read_fds);
					}
					else if (n > 0) {
						// se primeste o comanda de subscribe/unsubscribe
						request = (struct request*)buffer;
						string id;
						struct subscriber sub;
						for (auto& x : clients) {
							if (x.second == i) {
								id = x.first;
								break;
							}
						}

						sub.fd = i;
						sub.id = id;
						
						if (strcmp(request->command, "subscribe") == 0) {
							online_subsc[request->topic][sub] = request->sf;
						} else {
							online_subsc[request->topic].erase(sub);
						}
					}
                }
            }
        }

		if (close_server) {
			break;
		}
    }

	FD_CLR(udp_fd, &read_fds);
	FD_CLR(tcp_fd, &read_fds);
	FD_CLR(STDIN_FILENO, &read_fds);
    close(udp_fd);
	close(tcp_fd);
	fflush(stdout);
    return 0;
}