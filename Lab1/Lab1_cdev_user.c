/* Lab1_cdev_user.c
 * ECE4220/7220
 * Author: Luis Alberto Rivera
 * Modified by: Ramy Farag and Connor Johnson
 * Edited for Reading from Kernel Space and for Lab 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>

#define CHAR_DEV "/dev/Lab1" // "/dev/YourDevName"
#define MSG_SIZE 40

const int BLUE_LED_GPIO = 5;
const int GREEN_LED_GPIO = 4;
const int RED_LED_GPIO = 2;
const int YELLOW_LED_GPIO = 3;

int main(void)
{
    int cdev_id, bytes_read;
    char buffer[MSG_SIZE], ctrl;

    // Set up all LED outputs
    wiringPiSetupGpio();
    pinMode(BLUE_LED_GPIO, OUTPUT);
    pinMode(RED_LED_GPIO, OUTPUT);
    pinMode(GREEN_LED_GPIO, OUTPUT);
    pinMode(YELLOW_LED_GPIO, OUTPUT);

    // Open the Character Device for reading
    if((cdev_id = open(CHAR_DEV, O_RDONLY)) == -1) {
        printf("Cannot open device %s\n", CHAR_DEV);
        exit(1);
    }

    printf("cdev_id = %i\n", cdev_id);

    while(1)
    {
        // Reading from the character device
        bytes_read = read(cdev_id, buffer, sizeof(buffer)); 
        if (bytes_read < 0) {
            printf("Read failed, leaving...\n");
            break;
        }

        // Null-terminate the buffer to safely print it as a string
        buffer[bytes_read] = '\0';

        //printf("Bytes Read = %i, Read from device: %s\n", bytes_read, buffer);

        // Turn on LEDS
        ctrl = buffer[0];
        
        if(ctrl=='b'){
            digitalWrite(BLUE_LED_GPIO, HIGH);
        }
        else if(ctrl=='m'){
            digitalWrite(RED_LED_GPIO, HIGH);
        }
        else if(ctrl=='g'){
            digitalWrite(GREEN_LED_GPIO, HIGH);
        }
        else if(ctrl=='y'){
            digitalWrite(YELLOW_LED_GPIO, HIGH);
        }
        else if(ctrl=='r'){
            digitalWrite(BLUE_LED_GPIO, LOW);
            digitalWrite(RED_LED_GPIO, LOW);
            digitalWrite(YELLOW_LED_GPIO, LOW);
            digitalWrite(GREEN_LED_GPIO, LOW);
        }

    }

    close(cdev_id); // Close the device.
    return 0;
}
