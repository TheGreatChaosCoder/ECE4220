/* Lab6_cdev_user.c
 * ECE4220/7220
 * Author: Luis Alberto Rivera
 
 This program allows you to enter a message on the terminal, and then it writes the
 message to a Character Device. The Device should be created beforehand, as described
 in the Lab6_cdev_kmod.c file.
 Try this example together with the Lab6_cdev_kmod module. Once you enter a message, run
	dmesg | tail     ( | tail  is not needed. With that, you'll only see the last few entries).
 on a different terminal. You should see the message in the system log (printk'd in the
 module). That would mean that the message is getting to kernel space.
 Use elements from this example and the Lab6_cdev_user module in your Lab 6 programs. You may
 need to modify a bit the callback functions in the module, according to your needs.
 
 For the extra credit part of lab 6, you'll need to think about how to read messages coming
 from kernel space, and how to create those messages in the module...
*/

#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define CHAR_DEV "/dev/Lab6" // "/dev/YourDevName"
#define MSG_SIZE 40

// Sending message strings
char sMessage[40] = "# 128.206.22.108   \n";
char masterMessage[40] = "Connor is the master\n";

int masterFlag = 0;
int i, myVote, myIdx;
int recentVotes[20] = {0};

int cdev_id;

/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	/* This blindly prints the payload, but the payload can be anything so take care. */
	printf("%s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);
	char * buffer = msg->payload;
	printf("Received a datagram. It says: %s ", buffer);

    if ((buffer[15] != sMessage[15]))
    {
        if(0 == strcmp(buffer, "WHOIS\n"))
        {   
            if(masterFlag != 0){
                mosquitto_publish(mosq, NULL, "Election", sizeof(masterMessage),
								  masterMessage, 0, false);
                printf("Sending a datagram. It says: %s", masterMessage);
            }
        }
        else if(0 == strcmp(buffer, "VOTE\n"))
        {
            sMessage[17] = '0' + (rand()%10 + 1);
            recentVotes[(sMessage[14]-'0')*10 + (sMessage[15]-'0')] = sMessage[17] - '0';

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
		else if(buffer[0] == '@'){
			if(masterFlag != 1){
				write(cdev_id, buffer+1, sizeof(buffer[1]));
			}
		}
    }
}

int main(void)
{
	int rc;
	struct mosquitto * mosq;
	int dummy;
	char buffer[MSG_SIZE];
	
	// Open the Character Device for writing
	if((cdev_id = open(CHAR_DEV,O_RDWR)) == -1) {
		printf("Cannot open device %s\n", CHAR_DEV);
		exit(1);
	}
	
	// Connect to broker
	mosquitto_lib_init();
	
	mosq = mosquitto_new("publ", true, NULL);
	char *host="128.206.22.101";
	rc = mosquitto_connect(mosq, host, 1883, 6000);
	//rc = mosquitto_connect(mosq, "localhost", 1883, 7200);

	if(rc != 0){
		printf("Client could not connect to broker! Error Code: %d\n", rc);
		mosquitto_destroy(mosq);
		exit(1);
	}
	printf("We are now connected to the broker!\n");

	// Subscribe to Broker
	rc = mosquitto_subscribe(mosq, NULL, "Election", 1);
	
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}

	// Set message callback
	mosquitto_message_callback_set(mosq, on_message);

	while(1)
	{
		printf("Enter message (! to exit): ");
		fflush(stdout);
		gets(buffer);	// deprecated function, but it servers the purpose here...
						// One must be careful about message sizes on both sides.
		if(buffer[0] == '!')	// If the first character is '!', get out
			break;

		// Publish to Election topic
		mosquitto_publish(mosq, NULL, "Election", sizeof(buffer),buffer, 0, false);
		
		dummy = write(cdev_id, buffer, sizeof(buffer));
		if(dummy != sizeof(buffer)) {
			printf("Write failed, leaving...\n");
			break;
		}
	}

	// Disconnect from broker and clean up
	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	
	close(cdev_id);	// close the device.
	return 0;  		
}
