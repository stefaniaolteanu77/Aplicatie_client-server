#ifndef _UTILS_H
#define _UTILS_H 1
#include <iostream>
#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define BUFLEN 1600
#define MAX_CLIENTS 10


#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

struct udp_message {
	char topic[50];
	uint8_t data_type;
	char payload[1500];
}__attribute__((packed));

struct tcp_message {
	in_addr address;
	int num_port;
	char topic[50];
	uint8_t data_type;
	char payload[1500];
}__attribute__((packed));

struct request{
	char command[12];
	char topic[50];
	int sf;
}__attribute__((packed));

struct subscriber{
	string id;
	int fd;


	bool operator==(const subscriber&sub) const {
		return id == sub.id && fd == sub.fd;
	}
};

namespace std {
	template <>
	struct hash<subscriber>{
		size_t operator() (const subscriber&sub) const {
			size_t h1 = hash<string>()(sub.id);
			size_t h2 = hash<int>()(sub.fd);

			return h1^h2;
		}
	};
}


long long power(long long base, long long pow);
void make_server_message(in_addr udp_addr, int udp_port_num, struct udp_message* udp_message, struct tcp_message* tcp_message, char* type);
void usage_server(char *file);
void usage_subscriber(char *file);
void parse_command(char* buffer, struct request* request);

#endif
