/*
 * enum_general.h
 *
 * Created: 26.10.2016 13:17:36
 *  Author: TOE
 */ 


#ifndef ENUM_GENERAL_H_
#define ENUM_GENERAL_H_

typedef enum
{
	FRAME_STATUS_SCHEDULED = 1,
	FRAME_STATUS_WAITING,
	FRAME_STATUS_COMPLETE,
	FRAME_STATUS_ABANDONED,

	/* ADD NEW CODES HERE */
}__attribute__((packed)) status_t;



#endif /* ENUM_GENERAL_H_ */