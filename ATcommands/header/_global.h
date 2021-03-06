/*
 * _global.h
 *
 * Created: 26.10.2016 08:55:17
 *  Author: TOE
 */


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <inttypes.h>

// === miscellaneous ======================================
#define FLASH_ATTR

// === default values =====================================
#define AT_VERSION	         "1.0.10EF"  // Prototype Version
#define MAX_PARAMETER_LENGHT (20 + 2)    // +2 for buffer handling processes

#define DIRTYB_AP		     0x0001      // used in command AC
#define DIRTYB_BD		     0x0002      // used in command AC
#define DIRTYB_CT_AC	     0x0004      // used in command AC
#define DIRTYB_ID		     0x0008      // used in command AC
#define DIRTYB_CH		     0x0010      // used in command AC
#define DIRTYB_MAC_UPDATE	 0x0020      // used in send message function for MAC header update
#define DIRTYB_RO			 0x0040		 // used in AT & AP parser
#define DIRTYB_CC            0x0080		 // used in AT parser
#define DIRTYB_GT            0x0100		 // used in AT parser
#define DIRTYB_CT_AT         0x0200      // used in AT parser
#define DIRTYB_PL            0x0400		 // used in command AC

#define DIRTYB_ALL_ACTIVE    0x7FF

// === typedef ============================================
typedef enum { FALSE, TRUE }__attribute__((packed)) bool_t;
typedef enum { TRANSPARENT_MODE, AP_MODE, AP_ESCAPE_MODE }__attribute__((packed)) device_mode;

// === shared globals =====================================
extern uint16_t dirtyBits;

#endif /* _GLOBAL_H_ */