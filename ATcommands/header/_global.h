/*
 * _global.h
 *
 * Created: 26.10.2016 08:55:17
 *  Author: TOE
 */ 


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

// === miscellaneous ======================================
#define DEBUG 0			// debug mode ( 0 = off / 1 = on )
#define FLASH_ATTR

// === default values =====================================
#define AT_VERSION	"0.3.10E7"
#define MAXBYTES	(100)

// === typedef =========================================
typedef enum { FALSE, TRUE }__attribute__((packed)) bool_t;
typedef enum { TRANSPARENT_MODE, AP_MODE, AP_ESCAPE_MODE }__attribute__((packed)) device_mode;



#endif /* _GLOBAL_H_ */