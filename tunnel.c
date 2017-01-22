#include <netinet/in.h>
#include <time.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXLINE     4096    /* max text line length */
#define LISTENQ     1024    /* 2nd argument to listen() */
//#define DAYTIME_PORT 3333

char *mystrcpy(char *dst, const char *src){ //project 0
	int i = 0;
	for(i = 0; src[i] != '\0'; i++){
		dst[i] = src[i];
	}
	if(src[i] == '\0')
		dst[i] = src[i];
	return dst;
}

//inverse ip_to_host not needed ?
int host_to_ip(char *host)//converts to IPv4 if not already IPv4
{ 
	struct addrinfo *info, *info_list;
	struct sockaddr_in *addr;

	if((getaddrinfo(host, "https", NULL, &info)) != 0){ //Put address info into 'info'
		printf("Error getting address information\n");
		return 1;
	}
	// check each link and connect to the first one possible
	for(info_list = info; info_list != NULL; info_list = info_list->ai_next) {
		addr = (struct sockaddr_in*) info_list->ai_addr;
		mystrcpy(host , inet_ntoa(addr->sin_addr)); //inet_ntao converts host address to IPv4
	}
	//printf("host: %s\n", host);
	freeaddrinfo(info);
	return 0;
}

int main(int argc, char **argv){
	int listenfd, connfd, sockfd, port_num, len, n, arg;
	struct sockaddr_in tunnaddr, client;
	struct sockaddr_in* copy_client;
	char buff[MAXLINE];
	char recvline[MAXLINE + 1];
	char client_name[1024];
	char client_addr[1024];

	char serv_addr[1024];

	struct sockaddr_in servaddr; //socket.h
	struct sockaddr_in *copy_server;

	char server_name[MAXLINE];
	int server_port;

	time_t ticks;

	if (argc < 2) { //use 1 and only 1 argument -> give it a port
		printf("usage: port\n");
		exit(1);
	}

	port_num = atoi(argv[1]);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&tunnaddr, sizeof(tunnaddr));

	tunnaddr.sin_family = AF_INET;
	tunnaddr.sin_addr.s_addr = htonl(INADDR_ANY); //host byte to network byte order
	tunnaddr.sin_port = htons(port_num); /* daytime server */
	
	bind(listenfd, (struct sockaddr *) &tunnaddr, sizeof(tunnaddr));
	
	listen(listenfd, LISTENQ); //listenfd becomes passive (listener) with max Queue/buffer LISTENQ
	
	len = sizeof(client);
	arg = 0;
	for ( ; ; ) {
		connfd = accept(listenfd, (struct sockaddr *) &client, &len);
		
		copy_client = (struct sockaddr_in*) &client; //prevents seg fault
		struct in_addr copy_addr = copy_client->sin_addr;
		inet_ntop(AF_INET, &copy_addr, client_addr, 1024);

		//inet_ntop(AF_INET, client.sin_addr.s_addr, client_addr, 1024); //seg fault
		getnameinfo(copy_client, 1024, client_name, 1024, NULL, 0, 0);

		printf("Client Name: %s\n", client_name);//optional in tunnel
		printf("Client IP Address: %s\n", client_addr);

		ticks = time(NULL);

		snprintf(buff, sizeof(buff), "\n");
		write(connfd, buff, strlen(buff));//send back to the client

		while ((n = read(connfd, recvline, MAXLINE)) > 0) { //keep reading until EOF
			recvline[n] = 0;        /* null terminate */
			if (fputs(recvline, stdout) == EOF) {
			    printf("fputs error\n");
			    exit(1);
			}
			if(arg == 0){ //get server name
				arg = 1;
				mystrcpy(server_name, recvline);//do not share pointers
				
				if(host_to_ip(server_name) == 1){
					printf("Unable to convert Server Name to IPv4\n");
					return 1;
				}
			}
			else{
				server_port = atoi(recvline);
				//printf("Connecting to server: %s\n", server_name);
				goto CONNECT_TO_SERVER;
			}
		}

		CONNECT_TO_SERVER: //begin connection to server
			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //return if -1 (fail)
				printf("socket error\n");
				exit(1);
			}
			//printf("WHILE CONNECTING WITH: %s\n", server_name);
			bzero(&servaddr, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(server_port);
			if (inet_pton(AF_INET, server_name, &servaddr.sin_addr) <= 0) {
				printf("inet_pton error for %s\n", server_name);
				exit(1);
			}
			if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
				printf("connect error\n");
				exit(1);
			}

			copy_server = (struct sockaddr_in*) &servaddr; //prevents seg fault
			struct in_addr copy_addr_re = copy_server->sin_addr;
			inet_ntop(AF_INET, &copy_addr_re, serv_addr, 1024);
			getnameinfo(copy_server, 1024, server_name, 1024, NULL, 0, 0);
			getsockname(sockfd, copy_server, sizeof(copy_server));

			printf("\nServer Name: %s\n", server_name);
			printf("Server IP Address: %s\n", serv_addr);
			printf("Sending to Client...\n");

			arg = 0;
			while ((n = read(sockfd, recvline, MAXLINE)) > 0) { //keep reading until EOF
				recvline[n] = 0;        /* null terminate */
				if (fputs(recvline, stdout) == EOF) {
				    printf("fputs error\n");
				    exit(1);
				}
				if(arg == 1){
					arg++;
					mystrcpy(client_name, recvline);
				}
				else if(arg > 1){
					mystrcpy(client_addr, recvline);
				}
				arg++;
			}
			if (n < 0) { // should be 0 when done, >0 while executing read
				printf("read error\n");
				exit(1);
			}
			//client reads ONCE
			snprintf(buff, sizeof(buff), "Server Name: %s\n", server_name);
			write(connfd, buff, strlen(buff));
			snprintf(buff, sizeof(buff), "IP Address: %s\n", serv_addr);
			write(connfd, buff, strlen(buff));
			snprintf(buff, sizeof(buff), "Time: %.24s\r\n\n", ctime(&ticks));
			write(connfd, buff, strlen(buff));

			snprintf(buff, sizeof(buff), "Via Tunnel: %s\n", client_name);
			write(connfd, buff, strlen(buff));
			snprintf(buff, sizeof(buff), "IP Address: %s\n", client_addr);
			write(connfd, buff, strlen(buff));
			snprintf(buff, sizeof(buff), "Port Number: %hu\n", copy_client->sin_port);
			write(connfd, buff, strlen(buff));
		arg = 0;
		close(connfd);
	}
}
//host: csil-cpu5.csil.sfu.ca 
//(if connections error, change the port(s))



