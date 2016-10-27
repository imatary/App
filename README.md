# README ATcommands
Basic Firmware which supported same AT commands of XBee modules and make it posible to controll such modules via AT.

---

#### Which Controller are supported

* ARM Cortex M0

#### Miscellaneous

* created in [ Atmel Studio 7 ] [1]

#### Structure

/header  
/header/\_global.h  
/header/circularBuffer.h  
/header/cmd.h  
/header/defaultConfig.h  
/header/enum\_cmd.h  
/header/enum\_error.h  
/header/enum\_general.h  

/base  
/base/circularBuffer.c  
/base/setter.c

/stackRelated  
/stackRelated/stackrelated.h  
/stackRelated/trx0.c  
/stackRelated/uart0.c  

/atcommands.c (main file)  
/README.md  

#### Links

[1]: http://www.atmel.com/tools/atmelstudio.aspx#download "Atmel Studio 7"

created by [ http://dresden-elektronik.de/](http://dresden-elektronik.de/)
