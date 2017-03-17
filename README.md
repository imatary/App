# README ATcommands
Basic Firmware which supported same AT commands of XBee modules and make it possible to control such modules via AT.

---

#### Supported Controller

* ARM Cortex M0

#### Miscellaneous

* created in [ Atmel Studio 7 ] [1]
* current build created for [ µracoli stack ] [2] 
 * based on IEEE 802.15.4 protocol
 * 0.5.x [ repository version ] [3]

#### Structure

ATcommands/at\_api\_main.c (main file)  

ATcommands/base/ap\_exec.c  
ATcommands/base/ap\_frame.c  
ATcommands/base/ap\_local.c  
ATcommands/base/ap\_parser.c  
ATcommands/base/ap\_read.c  
ATcommands/base/ap\_trx.c  
ATcommands/base/ap\_write.c  
ATcommands/base/at\_exec.c  
ATcommands/base/at\_parser.c  
ATcommands/base/at\_read.c  
ATcommands/base/at\_write.c  
ATcommands/base/attable.c  
ATcommands/base/circularBuffer.c  
ATcommands/base/helper.c  
ATcommands/base/nvm_eeprom1.c  
ATcommands/base/rfmodul.c  
ATcommands/base/tp\_trx.c  

ATcommands/header/\_global.h  
ATcommands/header/ap\_frames.h  
ATcommands/header/at\_commands.h  
ATcommands/header/circularBuffer.h  
ATcommands/header/cmd.h  
ATcommands/header/defaultConfig.h  
ATcommands/header/enum\_cmd.h  
ATcommands/header/enum\_status.h  
ATcommands/header/helper.h  
ATcommands/header/rfmodul.h  

ATuracoli/trx0.c   
ATuracoli/uart0.c  
ATuracoli/timer0.c  
ATuracoli/stackrelated.h  
ATuracoli/stackrelated\_timer.h  
ATuracoli/stackdefines.h  

/README.md  

#### Version Number
The AT command version number contains two version numbers at once.
* the first part is the version number of the AT command API
* the second part consists the firmware version on which the AT command API is based

#### Version 1.0.10EF (end of internship)
* a bug which prevents to handle received frames was solved
* the firmware version (ATVR) was changed to 0xde10
* the hardware version (ATHV) was changed to 0x0
 * consequence: the module can be read by X-CTU Tool only with dresden elektronik add on (not included in GIT folder)
* the UART baud rate works now correctly
* the power level setting is now included
* ATWR updates data only if it has been modified
* several minor bugs are solved

#### Version 0.4.10EF
* AT and API mode are running now with state machine parser
* timer to enter and leave AT mode are equal to XBee implementation
* add TX status API frames
* add TX transmit API frames with 64-bit and 16-bit destination address
* add missing RX receive API frames, except RX (Receive) Packet IO
* adding support for broadcast messages
* speed up writing process

#### Version 0.3.10EF
* configurable in AT & AP mode (XCTU, HTerm)
* configurable in AT mode PuTTY (other programs are not tested)
* set PAN ID
* set channel
* set destination address (64-bit)
* read own IEEE address 64-bit (ConBee)
* set node identifier
* write parameter to EEPROM
* reset to default
* set the Command Mode Timeout
* stored the symbol to enter the AT command mode
* sending of simple text messages (Point2Point)
 * current version support only a package size of 127 bytes, everything else get lost
* sending of remote AT commands (Point2Point)
* received text messages
* received &ldquo;API Frames&rdquo; over air, except RX (Receive) Packets

#### H Files
1. \_global.h  
 * declaration of global variable `uint16_t dirtyBits`  
 * definitions of dirty bits status values  
 * definition of prototype version string  
 * type definition of enumerated bool values `typedef enum bool_t`  
 * type definition of enumerated device modes `typedef enum device_mode`  

2. ap\_frames.h  
 * declaration of prototypes for API frame functions  

3. at\_commands.h  
 * declaration of prototypes for AT command functions 

4. circularBuffer.h  
 * definition of buffer size  
 * type definition of enumerated buffer types `bufType_n`  
 * declaration of prototypes for buffer functions  

5. cmd.h  
 * defines of read, write and execute actions  
 * definition of typedef struct `CMD`  
 * declaration of prototypes for finding commands in table  
 
6. defaultConfig.h  
 * defines of standard values (XBee based)  
 * if you add new XBee modules please add the standard values in this file  

7. enum\_cmd.h  
 * enumeration of all AT commands  
 * if you want to add new AT commands, this file needs to be edited  

8. enum\_status.h  
 * enumeration of status and error messages

9. helper.h  
 * declaration of prototypes for validation functions  

10. rfmodul.h  
 * type definition of structure `RFmodul` (stored all parameters of the module in flash memory)
 * declaration of prototypes for set and get functions
 * declaration of prototypes for reading from EEPROM and writing to the EEPROM

11. stackrelated.h – Stack dependency
 * declaration of prototype pointer to UART functions
 * declaration of prototype pointer to transceiver functions
 * prototypes of basic TRX and UART functions  
 
12. stackdefines.h
 * renewed defines to make it easier to replace stack related defines
 
13. stackrelated_timer.h
 * declaration of prototype pointer to Timer functions

#### C Files
1. at\_api\_main.c –-- main file with main function   
 * declaration and initialization of dirtyBits variable  
 * reads values of EEPROM and store it into FLASH  
 * configure the module for UART and transceiver operating
 * starting active waiting loop

2. ap\_exec.c  
 * definition of execution function for API frames  

3. ap\_frame.c  
 * definition of API frame structure `api_f`
 * initialization of `api_f frame`
 * definition of API frame access functions for api_f structure

4. ap\_local.c  
 * definition of API frame type values  
 * definition of mac mode values    
 * definition of UART API frame handle function   
 * definition of local response messages  
  \- local device response API frame (for special DE commands)  
  \- local AT command response API frame  
  \- TX status API frame  
  \- RX receive package API frame
  \- remote frame response API frame
 * definition of local remote frame handling  

5. ap\_parser.c  
 * definition of API frame parser with state machine

6. ap\_read.c  
 * definition of read function for API frames  

7. ap\_trx.c 
 * definition for API transceiver frames  
  \- TX transmit frame with 16-bit and 64-bit destination address 
  \- remote frame
  \- remote response frame

8. ap\_write.c  
 * definition of write function for API frames  

9. at\_exec.c  
 * definition of execution function for AT commands   

10. at\_parser.c  
 * definition of API frame parser with state machine

11. at\_read.c  
 * definition of read function for AT commands   

12. at\_write.c  
 * definition of write function for AT commands   

13. attable.c  
 * definition of AT command table, including:  
  \- name (string)  
  \- ID  
  \- address at offset in api_f structure  
  \- read, write, execution options  
  \- maximum command size
  \- total minimum and maximum values   
  \- set function pointer
  \- validation function pointer
 * definition of 'find in table' functions
 * if you want to add new AT commands, this file needs to be edited

14. circularBuffer.c  
 * type definition of buffer structure `deBuffer_t`
 * definition and initialization of a buffer array
 * definition of buffer functions

15. helper.c  
 * definition validation functions

16. nvm_eeprom1.c  
 * set user and default values functions
 * get function
 * EEPROM crc function

17. rfmodul.c  
 * definition of get and set functions for RFmodul structure

18. tp\_trx.c 
 * definition for printing received frames
 * definition to create a message frame for transceiver
 
19. timer0.c
 * initialize timer pointer
 * definition of 'Millisecond'
 
20. trx0.c
 * initialize of transceiver functions
 * handler for sending and receiving
 * functions to pack data packages
 
21. uart.c
 * initialize of UART pointer functions  
 * definition for printing status messages through UART  


#### Links

[1]: http://www.atmel.com/tools/atmelstudio.aspx#download "Atmel Studio 7"
[2]: http://uracoli.nongnu.org/ "µracoli stack"
[3]: http://hg.savannah.nongnu.org/hgweb/uracoli/ "repository version"

created by [ http://dresden-elektronik.de/](http://dresden-elektronik.de/)
