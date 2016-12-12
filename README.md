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
 * defines der Kommandoaktionen read, write und execute
 * definition of typedef struct `CMD`
 * if you want to add new AT commands, this file needs to be edited
5. defaultConfig.h
 * defines der Standartwerte für die Zurücksetzung der Module (XBee basierend)
 * werden weitere XBee Module hinzugefügt muss diese Datei erweitert werden
6. atlocal.h
•	Prototypen der Funktionen, die in anderen c-Files aufgerufen werden
7. enum_cmd.h
•	Enumeration aller AT Kommandos, die von der Firmware verarbeitet werden
•	Diese Datei muss editiert werden wenn neue Befehle hinzugefügt werden
8. enum_status.h
•	Enumeration der Status- und Fehlermeldungen
9. rfmodul.h
•	Definiert das typedef struct RFmodul (Speicherort im der Parameter im Flash)
•	Prototypen der default setter
•	Prototypen für das schreiben und lesen zum EEPROM
10. stackrelated.h – Stack abhängig
•	Prototypenzeiger der UART-Funktionen
•	Prototypenzeiger der Transreceiver-Funktionen
•	Prototypen der Funktionen, die in anderen c-Files aufgerufen werden
11. stackdefines.h
•	Neue defines der Stack defines um die Suche und das Ersetzen zu erleichtern
12. stackrelated_timer.h
•	Prototypenzeiger der Timer-Funktionen

#### C Files
1.	atcommands.c – Hauptdatei mit der main-Funktion
•	initialisiert die RF modulvariablen
•	liest den EEPROM aus und speichert die Daten in den Flash
•	initialisiert den UART und Transceiver Buffer
•	konfiguriert das Gerät für die einzelnen Operationen
•	startet die aktive Warteschleife
 ○	ruft die TRX Handler Funktion auf (10.2.9)
 ○	ruft die lokale AT Funktion auf (10.2.2)
 ○	ruft die UART API Frame Handler Funktion auf (10.2.10)
2.	atlocal.c – Funktionen für die Lokale AT Behandlung
•	Hauptfunktion für den AT Command Mode
•	Funktionen read, write und exec für die Lokale Abhandlung
 ○	Bei Erweiterung der Kommandos in auslagern oder in Templates umwandeln
3.	apiframe.c – Funktionen die, die API Frames behandeln
•	Hauptfunktion um die API Frames abzuarbeiten
•	Aufruf der Frame-Typ-Funktionen
 ○	0x08 & 0x09 local AT commands, ist in den rwx-Funktionen der atlocal.c Datei in-tegriert
 ○	0x18 specific device commands 
 ○	0x17 remote AT commands, ist in der trx0.c ausprogrammiert 
 ○	0x88 local AT response
 ○	0x97 remote AT response /* in Arbeit */
4.	attable.c – Tabelle mit den AT commandos
•	Auflistung aller Kommandos mit ihren Rechten (rwx) in einem Array
•	Diese Datei muss editiert werden wenn neue Befehle hinzugefügt werden
5.	circularBuffer.c – Buffer Funktionen
•	Ausprogrammierte Pufferfunktionen
6.	nvm_eeprom.c – Speicher- und Lesefunktion für den EEPROM
•	Definition des EEPROM struct NVM
•	Set user values und default Funktion
•	Get Funktion
•	EEPROM crc Funktionen
7.	setter.c – Default-Setter-Funktionen
•	setter um die Standardwerte in dem Flash wieder herzustellen
8.	timer0.c – definiert die Verbindung zu der Timer-Funktion des Stacks
•	Initialisierung Timer-Pointer
•	Definition der Millisekunden für das Modul
9.	trx0.c – definiert die Verbindung zu den Transceiver-Funktionen des Stacks
•	Initialisierungsfunktion für den Transreceiver
•	Sende- und Empfängerhändler
•	Funktionen um die Pakete, die gesendet werden zu packen
10.	uart.c – definiert die Verbindung zu den User-Interface-Funktionen des Stacks
 * init of UART pointer functions


#### Links

[1]: http://www.atmel.com/tools/atmelstudio.aspx#download "Atmel Studio 7"

created by [ http://dresden-elektronik.de/](http://dresden-elektronik.de/)
