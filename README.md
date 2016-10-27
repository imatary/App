# README ATcommands
Basic Firmware which supported same AT commands of XBee modules and make it posible to controll such modules via AT.

---

#### Which Controller are supported

* ARM Cortex M0

#### Miscellaneous

* created in [ Atmel Studio 7 ] [1]

#### Structure

/ATcommands
* /header  
  * /\_global.h  
  * /circularBuffer.h  
  * /cmd.h  
  * /defaultConfig.h  
  * /enum\_cmd.h  
  * /enum\_error.h  
  * /enum\_general.h  

* /base  
  * /circularBuffer.c  
  * /setter.c

* /atcommands.c (main file) 

/ATuracoli  
* /stackrelated.h  
* /trx0.c  
* /uart0.c  
 
/README.md  

#### Links

[1]: http://www.atmel.com/tools/atmelstudio.aspx#download "Atmel Studio 7"

created by [ http://dresden-elektronik.de/](http://dresden-elektronik.de/)
