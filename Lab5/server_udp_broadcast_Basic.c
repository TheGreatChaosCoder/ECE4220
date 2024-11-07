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
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
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
   char broadcastAddress[20];
   struct ifreq ifr;

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

    // Get RPi's IP Address
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
    ioctl(sock, SIOCGIFADDR, &ifr);

    strncpy(broadcastAddress, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), 14);

    printf("Address: %s\n", broadcastAddress);


    // Sending message strings
   char sMessage[40] = "# 128.206.22.103   \n";
   char masterMessage[40] = "Connor on 128.206.22.103 is the master\n";

   // Copy IP address into messages
   strncpy(sMessage + 2, broadcastAddress, 14);
   strncpy(masterMessage + 10, broadcastAddress, 14);

   int masterFlag = 0;
   int i, myVote, myIdx;
   int recentVotes[20] = {0};
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
                if(masterFlag != 0){
                    addr.sin_addr.s_addr = inet_addr(broadcastAddress);
                    sendto(sock, masterMessage, 40, 0, (struct sockaddr *)&addr, fromlen);
                    printf("Sending a datagram. It says: %s", masterMessage);
                }
            }
            else if(0 == strcmp(buffer, "VOTE\n"))
            {
                sMessage[17] = '0' + (rand()%10 + 1);
                recentVotes[(sMessage[14]-'0')*10 + (sMessage[15]-'0')] = sMessage[17] - '0';
                addr.sin_addr.s_addr = inet_addr(broadcastAddress);
                sendto(sock, sMessage, 40, 0, (struct sockaddr *)&addr, fromlen);
                printf("Sending a datagram. It says: %s", sMessage);
            }
            else if (buffer[0] == '#')
            {
                recentVotes[(buffer[14]-'0')*10 + (buffer[15]-'0')] = buffer[17] - '0';

                // Search through and ensure that I am the master
                masterFlag = 1;
                myIdx = (sMessage[14]-'0')*10 + (sMessage[15]-'0');
                myVote = recentVotes[myIdx];

                for(i = 0; i<20; i++){
                    //printf("Vote %i: %i\n", i, recentVotes[i]);
                    if(i == myIdx){
                        continue; // Don't compare with myself
                    }

                    if(recentVotes[i] > myVote || (recentVotes[i] == myVote && i > myIdx)) {
                        masterFlag = 0; // Lost vote, give up
                        break;
                    }
                }

                printf("Master Flag: %i\n", masterFlag); 

            }
          }


   }

   return 0;
 }
