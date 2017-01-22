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

/*
bind(listenfd, //assign listenfd the below address
(struct sockaddr *) &servaddr, //assign this above
sizeof(servaddr) //specifies size of addess structure of 2nd argument
)


struct sockaddr_in{  
    short sin_family;  
    unsigned short sin_port;  
struct in_addr sin_addr;  //ip address (NETWORK BYTE ORDER)
    char sin_zero[8];  
}; 

Given a Socket Address: get Host and Service

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
               char *host, socklen_t hostlen,
               char *serv, socklen_t servlen, int flags);

getnameinfo((struct sockaddr *) &servaddr, sizeof(servaddr),
               char *host, socklen_t hostlen,
               NULL, 0, int flags);

*/


int main(int argc, char **argv){
	int listenfd, connfd, port_num, len;
	struct sockaddr_in servaddr, client;
	struct sockaddr_in* copy_client;
	char buff[MAXLINE];
	char client_name[1024];
	char client_addr[1024];
	time_t ticks;

	if (argc < 2) { //use 1 and only 1 argument -> give it a port
		printf("usage: port\n");
		exit(1);
	}

	port_num = atoi(argv[1]);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //host byte to network byte order
	servaddr.sin_port = htons(port_num); /* daytime server */

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)); //bind to random port
	
	listen(listenfd, LISTENQ); //listenfd becomes passive (listener) with max Queue/buffer LISTENQ
	
	len = sizeof(client);
	for ( ; ; ) {
		connfd = accept(listenfd, (struct sockaddr *) &client, &len);
		
		copy_client = (struct sockaddr_in*) &client; //prevents seg fault
		struct in_addr copy_addr = copy_client->sin_addr;
		inet_ntop(AF_INET, &copy_addr, client_addr, 1024);

		//inet_ntop(AF_INET, client.sin_addr.s_addr, client_addr, 1024); //seg fault
		getnameinfo(copy_client, 1024, client_name, 1024, NULL, 0, 0);

		printf("Client Name: %s\n", client_name);
		printf("Client IP Address: %s\n", client_addr);

		ticks = time(NULL);

		snprintf(buff, sizeof(buff), "Time: %.24s\r\n", ctime(&ticks));
		write(connfd, buff, strlen(buff));

		printf("%s", buff);

		close(connfd);
	}
}
//host: csil-cpu5.csil.sfu.ca 
//(doesn't seem to work 100% of time; try different port)


