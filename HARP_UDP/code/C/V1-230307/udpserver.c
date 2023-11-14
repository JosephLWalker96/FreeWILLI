/*
	Simple UDP Server
*/

#include <stdio.h>
#include <winsock2.h>
#include <stdlib.h>
#include <string.h>

//#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 2000	//Max length of buffer
#define PORT 50000	//The port on which to listen for incoming data

#define NUM_UDP_CHAN 2
#if (NUM_UDP_CHAN==1)
#define BLKINTERVAL 3410
#endif
#if (NUM_UDP_CHAN==2)
#define BLKINTERVAL 1550
#endif
#if (NUM_UDP_CHAN==3)
#define BLKINTERVAL 930
#endif
#if (NUM_UDP_CHAN==4)
#define BLKINTERVAL 620
#endif

int main(int argc, char **argv)
{
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen , recv_len, lusec, usec, d, i, pcount, lcount;
	unsigned char buf[BUFLEN];
	WSADATA wsa;

	slen = sizeof(si_other);
	
	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		printf("Failed. Error Code : %d",WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");
	
	//Create a socket
	if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d" , WSAGetLastError());
	}
	printf("Socket created.\n");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( PORT );
	
	si_other = server;
	si_other.sin_addr.s_addr = inet_addr("192.168.100.220");
	
	
	//Bind
	if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d" , WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	puts("Bind done");
	
	for (i=0;i<BUFLEN; i++) buf[i] = 0;
	strcpy(buf, "Open");
	
	if (sendto(s, buf, 100, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d" , WSAGetLastError());
			exit(EXIT_FAILURE);
		}
	
	//keep listening for data
	pcount = lcount = 0;
	while(1)
	{
	//	printf("Waiting for data...");
		fflush(stdout);
		
		//clear the buffer by filling null, it might have previously received data
		memset(buf,'\0', BUFLEN);
		
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d" , WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		
		pcount += 1;		// got another packet
		//print details of the client/peer and the data received
//		printf("Received packet from %s:%d len:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), recv_len);
		usec = (((unsigned int)buf[6])<<24) + (((unsigned int)buf[7])<<16) + (((unsigned int)buf[8])<<8) + ((unsigned int)buf[9]);
		d = usec - lusec; if (d < 0) d += 1000000;
		if (d != BLKINTERVAL) printf("Time Glitch: %02d/%02d/%02d %02d:%02d:%02d.%06d  %06d\n" , (int)buf[1], (int)buf[2], (int)buf[0], (int)buf[3], (int)buf[4], (int)buf[5], usec, d);
		lusec = usec;
		
		if (pcount >= 1000) {
			pcount = 0;
			printf(".");
			lcount += 1;
			if (lcount >= 50) {
				lcount = 0;
				printf("\n");
			}
		}
	
#if 0
		//now reply the client with the same data
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d" , WSAGetLastError());
			exit(EXIT_FAILURE);
		}
#endif
	}


	closesocket(s);
	WSACleanup();
}
