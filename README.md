ArduinoDMX facilitates DMX512 communication between user input and an Arduino Mega. Users can input command-line arguments within the Arduino IDE, and after entering them,
the light fixtures connected to the system respond accordingly.

The overall system can be broken down into three main sections: initialization, parsing user input, and sending data.

Initialization: 
* There are 20 addresses per channel, and each universe can only have a maximum of 25 channels.
* Channel numbers also always refer to the same address range, and these channel numbers increment every 20 addresses.
* The addresses associated with each channel cannot change.

Sending Data: DMX512 data is sent using the DMXSerial library by @mathertel [https://github.com/mathertel/DMXSerial].

****************************

INPUT: command-line input following these specifications: X/Y @ Z, where:
* X refers to the universe number, Y is the channel input, and Z is the intensity. (NOTE: if using more than one universe, there must be multiple DMX inputs into the Arduino.
* Channel input can be a singular channel (ex: 4), a range (4-7), a list (3,4,10,...), or any combination thereof.
* Intensity values must be between 0-100%

Acceptable inputs:
* 1/2 @ 50
* 2/3-7 @ 100
* 1/4,6,9,11 @ 30
* 1/2,3-5, 7 @ 50

Invalid inputs:
* 1/4 @ 110; intensity higher than 100%  
* 1/21,31 @ 45; channel number cannot be higher than 25
* 3 @ 100; no universe specified
* 2/5, 7, 10 @ 60; space after each comma in channel list

TO INPUT COMMANDS: must be compiled and run within the Arduino IDE environment; compile and then enter input into Arduino IDE command line (NOTE: can only enter one command at once.)

                                                                            
