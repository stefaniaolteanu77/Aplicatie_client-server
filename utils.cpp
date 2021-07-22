#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "utils.h"

// calculez pow(base, power) in O(log pow) cu recursivitate
long long power(long long base, long long pow) {
    long long temp;
    if (pow == 0) 
		return 1;
	temp = power(base, pow/2);

	if (pow % 2 == 0) 
		return temp * temp;
	else 
		return base * temp * temp;
}

// transformarea unui mesaj din formatul primit de la udp in formatul pe care vreau sa il trimit la tcp
void make_server_message(in_addr udp_addr, int udp_port_num, struct udp_message* udp_message, struct tcp_message* tcp_message, char* type) {
	
	// initializez campurile care nu trebuie modificate
	tcp_message->address = udp_addr;
	tcp_message->num_port = udp_port_num;
	memcpy(tcp_message->topic, udp_message->topic, sizeof(udp_message->topic));
	tcp_message->data_type = udp_message->data_type;
					
	long long integer;
	float short_real;
	float float_number;

	switch (udp_message->data_type) {
		case 0: {
			//primesc un INT
			integer = ntohl(*(uint32_t*)(udp_message->payload + 1));
			if (*udp_message->payload) {
				integer *= -1;
			}
			sprintf(type, "INT");
			sprintf(tcp_message->payload,"%lld",integer);
			break;
		}
					
		case 1: {
			// primesc un SHORT_REAL
			short_real = ntohs(*(uint16_t*)(udp_message->payload));
			short_real /= 100;
			sprintf(type, "SHORT_REAL");
			sprintf(tcp_message->payload,"%.2f",short_real);
			break;
		}
					
		case 2: {
			// primesc un FLOAT
			float_number = ntohl(*(uint32_t*)(udp_message->payload + 1));
			uint8_t pow = udp_message->payload[sizeof(uint32_t) + 1];
			float_number /= power(10, pow);
			if (*udp_message->payload) {
				float_number *= -1;
			}
			sprintf(type, "FLOAT");
			sprintf(tcp_message->payload,"%.4f",float_number);
			break;
			}
		
		case 3: {
			// primesc un STRING
			strcpy(type, "STRING");
			sprintf(tcp_message->payload,"%s",udp_message->payload);
			break;
		}	
		
		default:
			break;
	}

}

// parsarea componentelor unei comenzi in structura request
void parse_command(char* buffer, struct request* request) {

	char* substring = strtok(buffer, " ");
	DIE(substring == NULL, "not command");

	strcpy(request->command, substring);

	substring = strtok(NULL, " ");
	DIE(substring == NULL, "not command");
	
	strcpy(request->topic, substring);

	if (strcmp(request->command, "subscribe") == 0) {
		substring = strtok(NULL, " ");
		DIE(substring == NULL, "not command");
		request->sf = substring[0] - '0';
	}

} 

void usage_server(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}


void usage_subscriber(char *file)
{
	fprintf(stderr, "Usage: %s id_clinet server_address server_port\n", file);
	exit(0);
}