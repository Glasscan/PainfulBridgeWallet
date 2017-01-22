#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define MAXLINE  4096    /* max text line length 2^12*/
//#define DAYTIME_PORT 3333

/*

struct addrinfo {
	int              ai_flags;
	int              ai_family;
	int              ai_socktype;
	int              ai_protocol;
	socklen_t        ai_addrlen;
	struct sockaddr *ai_addr; //has address (Pointer)
	char            *ai_canonname;
	struct addrinfo *ai_next; // This is a linked-list(Pointer)
};

struct sockaddr_in{  
    short sin_family;  
    unsigned short sin_port;  
struct in_addr sin_addr;  //ip address
    char sin_zero[8];  
}; 

Given a Host and Service: Get Address Info

int getaddrinfo(const char *node, 
const char *service,
const struct addrinfo *hints,
struct addrinfo **res);

getaddrinfo("csil-cpu6.csil.sfu.ca",


)

*/

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

//no longer uses argc/argv
int main(int argc, char **argv)
{
	int sockfd, n;// port_num;
	//char *IPAddr;
	char recvline[MAXLINE + 1];
	char server_addr[1024];
	char server_name[1024];
	char buff[1024];

	struct sockaddr_in servaddr; //socket.h
	struct sockaddr_in *copy_server;

	if (argc < 2) { //use 1 and only 1 argument -> give it an IP address
		printf("usage: client <IPaddress>, port\n");
		exit(1);
	}

	if(argc == 5)//proceed to TUNNEL implementation
		goto TUNNEL;
//No Tunnel
	if(host_to_ip(argv[1]) == 1){ //invalid address
		printf("Invalid hostname or address \n");
		return 1;
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //return if -1 (fail)
		printf("socket error\n");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr)); //set bytes to 0 from &servaddr to its size
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));  /* daytime server -> convert string to int*/
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) { //arpa/inet.h -> return if failed
		printf("inet_pton error for %s\n", argv[1]);
		exit(1);
	}
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		printf("connect error\n"); //return on fail
		exit(1);
	}

	copy_server = (struct sockaddr_in*) &servaddr; //prevents seg fault
	struct in_addr copy_addr = copy_server->sin_addr;
	inet_ntop(AF_INET, &copy_addr, server_addr, 1024);
	getnameinfo(copy_server, 1024, server_name, 1024, NULL, 0, 0);

	printf("Server Name: %s\n", server_name);
	printf("Server IP Address: %s\n", server_addr);

	while ((n = read(sockfd, recvline, MAXLINE)) > 0) { //keep reading until EOF
		recvline[n] = 0;        /* null terminate */
		if (fputs(recvline, stdout) == EOF) {
		    printf("fputs error\n");
		    exit(1);
		}
	}
	if (n < 0) { // should be 0 when done, >0 while executing read
		printf("read error\n");
		exit(1);
	}

	exit(0);

//Using the Tunnel
	TUNNEL:
	
	if(host_to_ip(argv[1]) == 1){ //invalid address
		printf("Invalid hostname or address \n");
		return 1;
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //return if -1 (fail)
		printf("socket error\n");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr)); //set bytes to 0 from &servaddr to its size
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));  /* daytime server -> convert string to int*/
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) { //arpa/inet.h -> return if failed
		printf("inet_pton error for %s\n", argv[1]);
		exit(1);
	}
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		printf("connect error\n"); //return on fail
		exit(1);
	}

	//get tunnel connection info
	copy_server = (struct sockaddr_in*) &servaddr; //prevents seg fault
	struct in_addr copy_addr_tunn = copy_server->sin_addr;

	inet_ntop(AF_INET, &copy_addr_tunn, server_addr, 1024);
	getnameinfo(copy_server, 1024, server_name, 1024, NULL, 0, 0);
	getsockname(sockfd, copy_server, sizeof(copy_server));

	while ((n = read(sockfd, recvline, MAXLINE)) > 0) { //keep reading until EOF
		recvline[n] = 0;        /* null terminate */
		if (fputs(recvline, stdout) == EOF) {
		    printf("fputs error\n");
		    exit(1);
		}
		snprintf(buff, sizeof(buff), "%s", argv[3]);
		write(sockfd, buff, strlen(buff));//send address of server
		snprintf(buff, sizeof(buff), "%s", argv[4]);
		write(sockfd, buff, strlen(buff));//send port of server
	}

	exit(0);

}

//ssh -p 24 jmw35@csil-cpu5.csil.sfu.ca
//tunnel example input: csil-cpu5.csil.sfu.ca 3456 csil-cpu4.csil.sfu.ca 3421 


