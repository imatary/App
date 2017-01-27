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
#define AT_VERSION	         "0.4.10E7"
#define MAXBYTES			 100
#define MAX_PARAMETER_LENGHT 21

#define DIRTYB_AP		          0x0001 // used in command AC
#define DIRTYB_BD		          0x0002 // used in command AC
#define DIRTYB_CT		          0x0004 // used in command AC
#define DIRTYB_ID		          0x0008 // used in command AC
#define DIRTYB_CH		          0x0010 // used in command AC
#define DIRTYB_DH_DL	          0x0020 // used in send message function for MAC header update
#define DIRTYB_MY				  0x0040 // used in send message function for MAC header update

// === typedef ============================================
typedef enum { FALSE, TRUE }__attribute__((packed)) bool_t;
typedef enum { TRANSPARENT_MODE, AP_MODE, AP_ESCAPE_MODE }__attribute__((packed)) device_mode;

// === shared globals =====================================
uint8_t dirtyBits = 0;


#endif /* _GLOBAL_H_ */