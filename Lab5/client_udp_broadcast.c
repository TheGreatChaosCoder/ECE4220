/* 	Name        : 	client_udp_broadcast.c
	Authors     : 	Luis A. Rivera 
	Modified By : 	Ramy Farag
	Description: 	Simple client (broadcast)
	Course:			ECE4220/7220		*/

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
   int sock, n;
   unsigned int length;
   struct sockaddr_in anybody, from;
   char buffer[MSG_SIZE];	// to store received messages or messages to be sent.
   int boolval = 1;			// for a socket option
   char broadcastAddress[20] = "128.206.23.255";
   char *ipAddress;
   struct ifreq ifr;

   if (argc != 2)
   {
	   printf("usage: %s port\n", argv[0]);
       exit(1);
   }

   sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
   if (sock < 0)
	   error("socket");

   // change socket permissions to allow broadcast
   if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &boolval, sizeof(boolval)) < 0)
     	{
     		printf("error setting socket options\n");
     		exit(-1);
     	}

   anybody.sin_family = AF_INET;			// symbol constant for Internet domain
   anybody.sin_port = htons(atoi(argv[1]));	// port field
   											// broadcast address, you may need to change it (check ifconfig)
   anybody.sin_addr.s_addr =  INADDR_ANY;  // inet_addr("10.14.1.255"); // inet_addr("128.206.23.255");

   length = sizeof(struct sockaddr_in);		// size of structure

	// // You have to bind to the broadcast address to allow servers listen to each other
	//  if (bind(sock, (struct sockaddr *)&anybody, length) < 0)
	//  	error("binding");

	// Get RPi's IP Address
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
    ioctl(sock, SIOCGIFADDR, &ifr);

    ipAddress = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

    printf("Address: %s\n", ipAddress);


  do
  {
	  // bzero: to "clean up" the buffer. The messages aren't always the same length...
	  bzero(buffer,MSG_SIZE);		// sets all values to zero. memset() could be used
	  printf("Please enter the message (! to exit): ");
	  fgets(buffer,MSG_SIZE-1,stdin); // MSG_SIZE-1 because a null character is added


	  anybody.sin_addr.s_addr =  inet_addr(broadcastAddress); 
	  
	  if (buffer[0] != '!')
	  {
		  // send message to anyone there...
		  n = sendto(sock, buffer, strlen(buffer), 0,
				  (const struct sockaddr *)&anybody,length);
		  if (n < 0)
			  error("Sendto");

	  }
  } while (buffer[0] != '!');

   close(sock);						// close socket.
   return 0;
}
