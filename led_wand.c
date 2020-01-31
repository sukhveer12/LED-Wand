/*
 * File: led_wand.c
 * Author: Sukhveer Sahota
 *
 * Created on December 2nd, 2017
 *
 * Description: This project involved developing an LED Display Wand,
 * in which the user swings a breadboard containing 8 LEDs like an
 * inverted pendulum, and the LEDs will light up in such a way to 
 * display a message. 
 * 
 */

#include <xc.h>

// Macro used for the calculations of __delay_ms())
#define _XTAL_FREQ 1000000

// Define the Boolean datatype
#define bool int
#define true 1
#define false 0

// Constant that represents the analog output of the
// accelerometer when the breadboard is swung to the left
#define LEFTWARD_VOLTAGE_REFERENCE 0b10101011

__CONFIG(MCLRE_OFF & CP_OFF & CPD_OFF & BOREN_OFF & WDTE_OFF & PWRTE_OFF & FOSC_INTRCIO);

void delay(int delayTime);
void runLEDSequence();
void updateAverageTime(int timeToAdd);
void convertMessageStringToSegments();
void interrupt ISR(void);

// String storing the message to be displayed
unsigned char messageString[9];
// Length of the above String (i.e. array of char's)
// Not all 9 bytes will get used, but they are
// preallocated. The size will change throughout
// the course of the program, based on the wants
// of the user.
int lengthOfMessageString = 0;

// Array storing the segments of each character
// in the displayed message
unsigned char letterSegments[35];
// Number of message segments (required since not
// all 35 allocated bytes will get used)
int lengthOfMessage = 0;

// The time to display each segment of the message
int timePerSegment = 100;

// Variable used to implement the delay() function
// Counts how many for loop iterations have passed
int delayingVariable = 0;

// Boolean storing whether the sequence is running
bool isSequenceRunning = false;

int main(void) {           
    // Disable the analog select features by writing 0 to the appropriate registers
    ANSEL = 0b00000010;
    ANSELH = 0;
    
    // Initialize RA1 as an input
    TRISAbits.TRISA1 = 1; 
    
    // Initialize all 8 bits of PORTC as outputs
    TRISC = 0; 
    PORTC = 0;
    
    // 1 V is going forward, causes the comparator to output 1
    // 2 V is going backward, causes the comparator to output 2
    
    // Initialize Comparator C1
    CM1CON0 = 0b10000100;
    
    // Set the voltage reference of the comparator to 1V reference 
    // to detect a leftward movement
    VRCON = LEFTWARD_VOLTAGE_REFERENCE; 
    
    // Initialize the Comparator C1 Interrupt, which is generated
    // every time Comparator C1's output changes.
    PIE2bits.C1IE = 1;
    PIR2bits.C1IF = 0;
    INTCONbits.PEIE = 1;
    
    // Initialize the messageString
    messageString[0] = 'S';
    messageString[1] = 'A';
    messageString[2] = 'H';
    messageString[3] = 'O';
    messageString[4] = 'T';
    messageString[5] = 'A';
    // The current length of the msasge is 6
    lengthOfMessageString = 6;
    
    // Call the helper function convertMessageStringToSegments(),
    // which deciphers the above messageString array and generates
    // the corresponding letterSegments array
    convertMessageStringToSegments();
    
    // Delay for 1 second to allow all the analog signals from
    // the accelerometer to "settle"
    __delay_ms(1000);
    
    // Enable the Global Interrupt bit
    INTCONbits.GIE = 1;
    
    // Initialize TMR1, which is used for timing purposes
    TMR1H = 0;
    TMR1L = 0;
    T1CON = 0b10110001;

    // Infinite while loop that displays the sequence
    // whenever the isSequenceRunning flag is set to true
    while (1) {       
        if (isSequenceRunning) {
            runLEDSequence();
            isSequenceRunning = false;
        }
    }

    return 0;
}

// This is the Interrupt Service Routine, which is called
// every time the output of Comparator C1 changes (i.e. when
// the user swings the breadboard).
void interrupt ISR(void) {
    // The following explains how the accelerometer's output
    // is interpreted:
    // 
    // When the accelerometer is at rest, the voltage on the Y_OUT
    // is about 1.3V. When the accelerometer is moving to the left,
    // the output drops to a bit below 1.0V. When the accelerometer
    // begins moving to the right again, the voltage starts to increase
    // again to above 1.0V. Thus, we can say that whenever the accelerometer's
    // output goes from below 1.0V to above 1.0V (or vice-versa), the user has
    // completed a full swing (starting from the right side, swung left,
    // then swung right again = 1 full swing).
    // To detect this change around 1.0V, we can use a comparator with its
    // reference set to 1.0V. The internal comparator (Comparator C1) is used.
    //
    // The output of the comparator is as follows:
    // If C1OUT == 1, it means that the input voltage is lower than the
    //                reference voltage.
    // If C1OUT == 0, it means that the input voltage is greater than the
    //                reference voltage.
    // In other words:
    // If C1OUT == 1, the board is being accelerated to the left.
    // If C1OUT == 0, the board is being accelerated to the right.
    //
    // Whenever C1OUT changes, a full swing has occurred. Thus, by making
    // it so that this interrupt is called whenever C1OUT changes, we
    // can detect whenever a swing has been completed (and, equivalently, when
    // a new swing has been initiated).
    //
    // This logic is the basis for the code in this function (along with 
    // a few other algorithms including one for "debouncing").
    
    
    // Read the value of TMR1 and determine how long it has been
    // since the last interrupt was triggered
    T1CONbits.TMR1ON = 0;
    int duration = TMR1L + (256 * TMR1H);
    T1CONbits.TMR1ON = 1;

    // Rightward acceleration = End of Sequence (assuming a valid trigger)
    if (CM1CON0bits.C1OUT == 0) {
        // "Debounce" routine used to determine if this interrupt
        // trigger is "legit" (full swing takes at least 14000 for loop
        // iterations).
        if (duration > 14000) {            
            // If it is "legit", then stop the sequence from continuing
            // using the delayingVariable and timePerSegment variables
            // (this will cause it to end "early").
            timePerSegment = 0;
            delayingVariable = 1;
            
            // Reset TMR1 back to 0
            T1CONbits.TMR1ON = 0;
            TMR1H = 0;
            TMR1L = 0;
            T1CONbits.TMR1ON = 1;    
        }
    }
    
    // Leftward acceleration = Start of Sequence (assuming a valid trigger)
    else if (CM1CON0bits.C1OUT == 1) { 
        // "Debounce" routine used to determine if this interrupt
        // trigger is "legit" (time to go from rightward movement to
        // leftward movement in the process of initiating a new
        // sequence is at least 1500 for loop iterations).
        if (duration > 1500) {
            // If it is "legit", then update the timePerSegment variable
            // based on the data from the previous swing and then start
            // the sequence using the isSequenceRunningFlag.
            timePerSegment = duration / (2.4 * lengthOfMessage);
            isSequenceRunning = true;
            
            // Reset TMR1 back to 0
            T1CONbits.TMR1ON = 0;
            TMR1H = 0;
            TMR1L = 0;
            T1CONbits.TMR1ON = 1;   
        }
    }
    
    // Clear the C1IF flag, as this Interrupt Request
    // has been serviced
    PIR2bits.C1IF = 0;   
}

// Function used to run the sequence by displaying each segment
// in the letterSegments array sequentially for the calculated
// amount of time
void runLEDSequence() {
    for (int i = lengthOfMessage - 1; i >= 0; i--) {
        PORTC = letterSegments[i];
        delay(timePerSegment);
    }
    
    // After all the segments have been displayed, turn the LEDs off
    PORTC = 0;
}

// Function used to implement a custom delay
// This is required since the __delay_ms() macro doesn't
// allow for variable delay times, which is what is required
// in this application.
void delay(int delayTime) {
    for (delayingVariable = delayTime; delayingVariable > 0; delayingVariable--) {}
}

// Function used to decipher the above messageString array and generate
// the corresponding letterSegments array
void convertMessageStringToSegments() {
    // Variable storing the next empty index in the letterSegments array
    int indexOfLetterSegments = 0;
    
    // Iterate through all of the characters in the messageString
    for (int i = 0; i < lengthOfMessageString; i++) {
        // Use a switch statement to generate the appropriate
        // letter segments based on the character in the messageString
        switch (messageString[i]) {
            // Segments of a full Space
            case ' ':
                letterSegments[indexOfLetterSegments] =     0b00000000;
                letterSegments[indexOfLetterSegments] =     0b00000000;
                indexOfLetterSegments = indexOfLetterSegments + 2;
                break;
           
            // Segments for all of the uppercase letters
            case 'A':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00001001;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'B':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10010000;
                letterSegments[indexOfLetterSegments + 2] = 0b11110000;
                indexOfLetterSegments = indexOfLetterSegments + 3;                
                break;
            case 'C':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10000001;
                letterSegments[indexOfLetterSegments + 2] = 0b10000001;
                indexOfLetterSegments = indexOfLetterSegments + 3;                
                break;
            case 'D':
                letterSegments[indexOfLetterSegments] =     0b11110000;
                letterSegments[indexOfLetterSegments + 1] = 0b10010000;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'E':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10010001;
                letterSegments[indexOfLetterSegments + 2] = 0b10010001;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'F':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00001001;
                letterSegments[indexOfLetterSegments + 2] = 0b00001001;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'G':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10010001;
                letterSegments[indexOfLetterSegments + 2] = 0b11110001;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'H':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00001000;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'I':
                letterSegments[indexOfLetterSegments] =     0b10000001;
                letterSegments[indexOfLetterSegments + 1] = 0b11111111;
                letterSegments[indexOfLetterSegments + 2] = 0b10000001;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'J':
                letterSegments[indexOfLetterSegments] =     0b10000001;
                letterSegments[indexOfLetterSegments + 1] = 0b11111111;
                letterSegments[indexOfLetterSegments + 2] = 0b00000001;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'K':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00100100;
                letterSegments[indexOfLetterSegments + 2] = 0b01000010;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'L':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10000000;
                letterSegments[indexOfLetterSegments + 2] = 0b10000000;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'M':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00001111;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'N':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00000001;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'O':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10000001;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'P':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b00001001;
                letterSegments[indexOfLetterSegments + 2] = 0b00001111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'Q':
                letterSegments[indexOfLetterSegments] =     0b00111111;
                letterSegments[indexOfLetterSegments + 1] = 0b01100001;
                letterSegments[indexOfLetterSegments + 2] = 0b10111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'R':
                letterSegments[indexOfLetterSegments] =     0b11110000;
                letterSegments[indexOfLetterSegments + 1] = 0b00010000;
                letterSegments[indexOfLetterSegments + 2] = 0b00010000;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'S':
                letterSegments[indexOfLetterSegments] =     0b10011111;
                letterSegments[indexOfLetterSegments + 1] = 0b10010001;
                letterSegments[indexOfLetterSegments + 2] = 0b11110001;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'T':
                letterSegments[indexOfLetterSegments] =     0b00001000;
                letterSegments[indexOfLetterSegments + 1] = 0b11111111;
                letterSegments[indexOfLetterSegments + 2] = 0b00001000;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'U':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b10000000;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'V':
                letterSegments[indexOfLetterSegments] =     0b01100000;
                letterSegments[indexOfLetterSegments + 1] = 0b10000000;
                letterSegments[indexOfLetterSegments + 2] = 0b01100000;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'W':
                letterSegments[indexOfLetterSegments] =     0b11111111;
                letterSegments[indexOfLetterSegments + 1] = 0b11110000 ;
                letterSegments[indexOfLetterSegments + 2] = 0b11111111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'X':
                letterSegments[indexOfLetterSegments] =     0b11000011;
                letterSegments[indexOfLetterSegments + 1] = 0b00111100;
                letterSegments[indexOfLetterSegments + 2] = 0b11000011;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'Y':
                letterSegments[indexOfLetterSegments] =     0b00001111;
                letterSegments[indexOfLetterSegments + 1] = 0b11111000;
                letterSegments[indexOfLetterSegments + 2] = 0b00001111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
            case 'Z':
                letterSegments[indexOfLetterSegments] =     0b11100001;
                letterSegments[indexOfLetterSegments + 1] = 0b10011001;
                letterSegments[indexOfLetterSegments + 2] = 0b10000111;
                indexOfLetterSegments = indexOfLetterSegments + 3;
                break;
        }
        // After every character, insert a small space
        letterSegments[indexOfLetterSegments] = 0b00000000;
        letterSegments[indexOfLetterSegments] = 0b00000000;
        indexOfLetterSegments = indexOfLetterSegments + 2;
    }
    // After all the characters in the message have been converted into
    // segments, update the lengthOfMessage (i.e. number of segments) variable.
    lengthOfMessage = indexOfLetterSegments;
}
