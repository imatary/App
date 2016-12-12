/*
 * atlocal.h
 *
 * Created: 15.11.2016 11:33:30
 *  Author: TOE
 */ 


#ifndef ATLOCAL_H_
#define ATLOCAL_H_

#include "_global.h"
#include "enum_status.h"	// at_status_t
#include "apiframe.h"

// === prototypes ==========================================
/*
 * Main function which is called  by three plus types,
 * start of a new loop
 */
void	AT_localMode(void);

/*
 * Functions to read param, execute instructions or write to memory
 */
at_status_t CMD_readOrExec(struct api_f *frame, uint8_t *array, uint32_t *th);
at_status_t CMD_write(struct api_f *frame, uint8_t *array, size_t *len);
#endif /* ATLOCAL_H_ */