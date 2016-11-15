/*
 * ctemplates.h
 *
 * Created: 15.11.2016 09:22:23
 *  Author: TOE
 */ 


#ifndef CTEMPLATES_H_
#define CTEMPLATES_H_

#define deTEMPLATE(TYPE)\

void saveToMem_##TYPE( TYPE* dstInStruct, TYPE* min, TYPE* max, TYPE* data ) {\
\
	if ( data >= min && data <= max ) {\
		dstInStruct = data;\
		UART_print("OK\r\n");\
	}\
	else { UART_print("Invalid parameter!\r\n"); }\
}



#endif /* CTEMPLATES_H_ */