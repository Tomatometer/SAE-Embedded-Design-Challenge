#ifndef _HAL_H_
#define _HAL_H_
#include <stdint.h>


#define CAN_LEN 8 // A CAN message has 8 data bytes

// Send a stream of bytes on UART
void HAL_UartSend(uint8_t* data);

// Sets the LED to an RGB color (0-255 each)
void HAL_SetLED(uint8_t r, uint8_t g, uint8_t b);

// Read a CAN message from the bus. The ID is written 
// into the *id, and the message is written into *data
void HAL_ReadCanMessage(uint32_t* id, uint8_t* data);


#endif

