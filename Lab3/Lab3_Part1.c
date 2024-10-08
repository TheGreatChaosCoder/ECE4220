#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <pthread.h>

const int GREEN_LED_GPIO = 4;
const int RED_LED_GPIO = 2;
const int YELLOW_LED_GPIO = 3;
const int BLUE_LED_GPIO = 5;
const int WALK_BUTTON_GPIO = 16;

int main(void)
{
    // Set up all GPIOs
    wiringPiSetupGpio();

    pinMode(RED_LED_GPIO, OUTPUT);
    pinMode(GREEN_LED_GPIO, OUTPUT);
    pinMode(YELLOW_LED_GPIO, OUTPUT);
    pinMode(BLUE_LED_GPIO, OUTPUT);

    pinMode(WALK_BUTTON_GPIO, INPUT);
    pullUpDnControl(WALK_BUTTON_GPIO, PUD_DOWN);

    // Force Blue LED Off
    digitalWrite(BLUE_LED_GPIO, LOW);

    // Polled Scheudling
    unsigned int state = 0;
    while(1)
    {
        if(state == 0){
            digitalWrite(GREEN_LED_GPIO, HIGH);
            digitalWrite(YELLOW_LED_GPIO, LOW);
            digitalWrite(RED_LED_GPIO, LOW);
            state = 1;

            sleep(1);
        }
        else if(state == 1){
            digitalWrite(GREEN_LED_GPIO, LOW);
            digitalWrite(YELLOW_LED_GPIO, HIGH);
            digitalWrite(RED_LED_GPIO, LOW);
            state = 0;

            sleep(1);
        }

        if(digitalRead(WALK_BUTTON_GPIO) == HIGH){
            digitalWrite(GREEN_LED_GPIO, LOW);
            digitalWrite(YELLOW_LED_GPIO, LOW);
            digitalWrite(RED_LED_GPIO, HIGH);

            sleep(2);
        }
    }

    return 0;
}
