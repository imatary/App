# README ATcommands
Basic Firmware which supported same AT commands of XBee modules and make it posible to controll such modules via AT.

---

#### Which Controller are supported

* ARM Cortex M0

#### Miscellaneous

* created in [ Atmel Studio 7 ] [1]

#### Structure

ATcommands/atcommands.c (main file)

ATcommands/base/circularBuffer.c  
ATcommands/base/setter.c  
ATcommands/base/atlocal.c  
ATcommands/base/attable.c  
ATcommands/base/nvm_eeprom.c  
ATcommands/base/apiframe.c  

ATcommands/header/\_global.h  
ATcommands/header/rfmodul.h  
ATcommands/header/atlocal.h  
ATcommands/header/apiframe.h  
ATcommands/header/defaultConfig.h  
ATcommands/header/enum\_general.h  
ATcommands/header/enum\_error.h  
ATcommands/header/enum\_cmd.h  
ATcommands/header/circularBuffer.h  
ATcommands/header/cmd.h  

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
* the second part consists the firmware version on which the AT command API is besed

#### Version 0.1.10EF
* configurable in AT & AP mode (XCTU, HTerm)
* configurable in AT mode PuTTY (other programms are not tested)
* set PAN ID
* set channel
* set destination adress (64Bit)
* read own IEEE address 64Bit (ConBee)
* write params to EEPROM
* reset to default
* set the Command Mode Timeout
* stored the symbol to enter the AT command mode
* sending of simple text messages (Point2Point)
 * the current version supported only a package size  of 127Byte everthing else get lost
* sending of remote AT commands (Point2Point)
* received text messages
* received "API Frames" over air but the API sent it direktly to UART (no frame handling)

#### H Files
1. _global.h
 * definition of globals
2. apiframe.h
 * defines of API frame typs
 * definition of `api_f` struct
 * prototypes of functions which are called by other c-files
3. circularBuffer.h
 * definition of typdef struct `deBuffer_t`
 * defines of the buffer size and buffer mask
 * prototypes of all buffer functions
4. cmd.h
 * defines of read, write und execute actions
 * definition of typedef struct `CMD`
 * if you want to add new AT commands, this file needs to be edited
5. defaultConfig.h
 * defines of standard values (XBee based)
 * if you add new XBee modules please add the standard values in this file
6. atlocal.h
 * prototypes of functions which are called by other c-files
7. enum_cmd.h
 * enumeration of all AT commands
 * if you want to add new AT commands, this file needs to be edited
8. enum_status.h
 * enumeration of status and error messages
9. rfmodul.h
 * definition of typedef struct `RFmodul` (stored all params of the module in flash memory)
 * prototypes of default setter
 * prototypes for reading from and writing to the EEPROM
10. stackrelated.h – Stack dependency
 * prototype pointer to UART functions
 * prototype pointer to transreceiver functions
 * prototypes of functions which are called by other c-files
11. stackdefines.h
 * renewed defines to make it easier to replace stack related defines
12. stackrelated_timer.h
 * prototype pointer to Timer functions

#### C Files
1. atcommands.c – main file with main function
 * init of RF modul varibles
 * reads values of EEPROM and store it into FLASH
 * init UART and transceiver buffer
 * configure the modul for operrating
 * starting active waiting loop
2. atlocal.c – functions for local AT handling
 * main function of the  AT Command Mode
 * functions read, write und exec for local handling
3. apiframe.c – function for API handling
 * main fubction of API Frame handling
 * call of diverent frame type functions  
   \- 0x08 & 0x09 local AT commands, is defined in the rwx functions in atlocal.c  
   \- 0x18 specific device commands  
   \- 0x17 remote AT commands, is defined in trx0.c  
   \- 0x88 local AT response  
   \- 0x97 remote AT response /* work in progress */
4. attable.c – table of AT commands
 * list of all commands and their rights (rwx) in one array
 * if you want to add new AT commands, this file needs to be edited
5.	circularBuffer.c – buffer functions
 * programmed buffer functions
6.	nvm_eeprom.c – save and read functions for EEPROM operations
 * definition of EEPROM struct `NVM`
 * set user and default values functions
 * get function
 * EEPROM crc function
7.	setter.c
 * set functions to restore the default values
8.	timer0.c – defined the relation to the timer functions of the stack
 * init timer pointer
 * definition of 'Millisecond'
9.	trx0.c – defined the relation to the transceiver functions of the stack
 * init of transreceiver
 * handler for sending and receiving
 * functions to pack data packages
10.	uart.c – defined the relation to the user interface functions of the stack
 * init of UART pointer functions


#### Links

[1]: http://www.atmel.com/tools/atmelstudio.aspx#download "Atmel Studio 7"

created by [ http://dresden-elektronik.de/](http://dresden-elektronik.de/)
