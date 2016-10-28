/*
 * _global.h
 *
 * Created: 26.10.2016 08:55:17
 *  Author: TOE
 */ 


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

// === std. includes ======================================
#include <stdio.h>
#include <stdlib.h>

// === user includes ======================================
#include "cmd.h"
#include "enum_error.h"
#include "defaultConfig.h"

// === miscellaneous ======================================
#define DEBUG 1					// debug mode ( 0 = off / 1 = on )

// === default values =====================================
#define AT_VERSION	"0.1"
#define MAXBYTES	(100)

// === conditions =========================================
typedef enum { FALSE, TRUE }__attribute__((packed)) bool_t;

// === variables & structs ================================
typedef struct  {
	uint8_t  ch;
	uint16_t id;
	uint32_t dh;
	uint32_t dl;
	uint16_t my;
	uint32_t sh;
	uint32_t sl;
	bool_t	 ce;
	uint16_t sc;
	uint8_t	 ni;
}netCommand;

netCommand netCMD;

// === functions ==========================================
void SET_netDefault();
//ATERROR AT_localMode();


#endif /* _GLOBAL_H_ */