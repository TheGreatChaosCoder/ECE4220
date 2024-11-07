/* 	Name       : 	server_udp_broadcast.c
	Author     : 	Luis A. Rivera
	Description: 	Simple server (broadcast)
					ECE4220/7220		*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MSG_SIZE 40			// message size

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   int boolval = 1;			// for a socket option
   socklen_t fromlen;
   struct sockaddr_in server;
   struct sockaddr_in addr;
   char buffer[MSG_SIZE];	// to store received messages or messages to be sent.

   if (argc < 2)
   {
	  printf("usage: %s port\n", argv[0]);
      exit(0);
   }

   sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
   if (sock < 0)
	   error("Opening socket");

   length = sizeof(server);			// length of structure
   bzero(&server,length);			// sets all values to zero. memset() could be used
   server.sin_family = AF_INET;		// symbol constant for Internet domain
   server.sin_addr.s_addr = INADDR_ANY;		// IP address of the machine on which
											// the server is running
   server.sin_port = htons(atoi(argv[1]));	// port number

   // binds the socket to the address of the host and the port number
   if (bind(sock, (struct sockaddr *)&server, length) < 0)
       error("binding");

   // change socket permissions to allow broadcast
   if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &boolval, sizeof(boolval)) < 0)
   	{
   		printf("error setting socket options\n");
   		exit(-1);
   	}

    // Sending message string
   char sMessage[] = {"# 128.206.22.103   \n"};

   char broadcastAddress[] = "128.206.19.255"; //"10.14.1.255"
   int masterFlag = 0;
   fromlen = sizeof(struct sockaddr_in);	// size of structure

   while (1)
   {
       
	   // bzero: to "clean up" the buffer. The messages aren't always the same length...
	   bzero(buffer,MSG_SIZE);		// sets all values to zero. memset() could be used

	   // receive from a client
	   n = recvfrom(sock, buffer, MSG_SIZE, 0, (struct sockaddr *)&addr, &fromlen);
       if (n < 0)
    	   error("recvfrom"); 

       printf("Received a datagram. It says: %s ", buffer);

          if(n<0)
            error("recvfrom");

          if ((buffer[15] != sMessage[15]))
          {
            if(0 == strcmp(buffer, "WHOIS\n"))
            {   
                puts("WHOIS HAS BEEN DETECTED");
                if(masterFlag != 0){
                    addr.sin_addr.s_addr = inet_addr(broadcastAddress);
                    sendto(sock, "Connor is the master!!"\n, 40, 0, (struct sockaddr *)&addr, fromlen);
                }
            }
            else if(0 == strcmp(buffer, "VOTE\n"))
            {
                puts("VOTE HAS BEEN DETECTED");
                sMessage[17] = '0' + (rand()%9);
                addr.sin_addr.s_addr = inet_addr(broadcastAddress);
                sendto(sock, sMessage, 40, 0, (struct sockaddr *)&addr, fromlen);
                printf("Sending a datagram. It says: %s\n", sMessage);
            }
            else if (buffer[0] == '#')
            {
                puts("# HAS BEEN DETECTED");
                
                masterFlag = buffer[17] == sMessage[17] ?
                             (buffer[15] > sMessage[15] ? 0 : 1) :
                             (buffer[17] < sMessage[17] ? 1 : 0);

                printf("Master flag: %i\n", masterFlag); 

            }
          }


   }

   return 0;
 }
