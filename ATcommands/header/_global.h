/*
 * _global.h
 *
 * Created: 26.10.2016 08:55:17
 *  Author: TOE
 */ 


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

// === std. includes ======================================

// === miscellaneous ======================================
#define DEBUG 0			// debug mode ( 0 = off / 1 = on )
#define FLASH_ATTR

// === default values =====================================
#define AT_VERSION	"0.1"
#define MAXBYTES	(100)

// === conditions =========================================
typedef enum { FALSE, TRUE }__attribute__((packed)) bool_t;

// === variables & structs ================================

// === functions ==========================================


#endif /* _GLOBAL_H_ */