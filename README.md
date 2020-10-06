# LED-Wand
## Description
This project is a scaled-down version of an LED fan built using only a row of LEDs on a breadboard. It allows users to display an arbitrary message "in the air" while the  breadboard is swung back and forth. It was built using the PIC16F690 microcontroller.

## High-Level Technical Explanation
As the breadboard is swung, the device lights up the LEDs in a specific sequence. The sequence changes throughout the swing, and each sequence is appropriately timed to be displayed at the right time and for the right amount of time. As a result, when looking at the lights as they are swung, our brain puts the individual images of the LED sequences together in order to render the desired message.

The device uses an accelerometer to detect the motion of the breadboard and thus synchronize the LED sequence with the user’s swing so that a clean-looking message is displayed. It's assumed that the user swings the board with an approximately constant velocity for most of the swing and only accelerates abruptly at the ends of the swing when changing direction. Using this assumption, the device can check for the start and end of the swing by detecting large changes in the accelerometer output.

Internally, the microcontroller was configured with interrupts (to detect exactly when the swing starts/stops), timers (to measure the length of the swings), and a comparator (to detect large accelerations and ignore periods of constant velocity).

## Demonstration
In the end, our design worked very well. Messages were displayed very clearly, and the system adapted well to different users with different swinging speeds and distances.

Below is a video of some of the messages that we were able to display with our design.

[![Video demonstration of the LED Wand displaying the message "Hello"](blob/master/Thumbnail.png)](https://www.youtube.com/watch?v=kxvFPX3jv1A&feature=youtu.be "LED Wand Demonstration")
