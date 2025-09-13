#include "HAL.h"
#include <stdbool.h>
#include <stdint.h>

#define QUEUE_ORDER 11 //MAY NEED TO ADJUST BASED ON SPECIFIC MESSAGE RATE REQUIREMENTS
#define QUEUE_CAPACITY 1U << QUEUE_ORDER // 2 ** BUFFER_ORDER
#define QUEUE_MASK (QUEUE_CAPACITY - 1U) // Used as a faster wrap around than modulo

//Create a static circular buffer for CAN Messages to be processsed
static volatile uint32_t bufferHead; // Where the CAN Messages will be added
static volatile uint32_t bufferTail; // Where the CAN Messages will be read
static CanMessage buffer[QUEUE_CAPACITY];

typedef struct{
    uint8_t data[CAN_LEN];
    uint32_t id;
}CanMessage;


//Helper to determine the amound of elements in the buffer
inline static uint32_t bufferSize(void)
{
    uint32_t h = bufferHead;
    uint32_t t = bufferTail;
    return (h - t) & QUEUE_MASK;
}

void Init()
{
    bufferHead = bufferTail = 0; // No elements in the buffer

    HAL_SetLED(255, 0, 0); //No Messages are being processed yet
}

void Iter()
{
    if(bufferSize() > 0)
        // We have Messages to process, set LED to Green
        HAL_SetLED(0, 255, 0);
    else
        HAL_SetLED(255, 0 ,0);

    while(bufferHead != bufferTail)
    {
        //Read the index
        uint32_t tailIndex = bufferTail & QUEUE_MASK;

        // Process the Next CAN Message
        uint32_t messageID = buffer[tailIndex].id;
        
        //Copy the Message ID Stuff
        uint8_t messageData[CAN_LEN];
        for(int i = 0; i < CAN_LEN; i++) messageData[i] = buffer[tailIndex].data[i];

        // Advance the tail to the next value (we will not be using buffer tail again in the iteration)
        bufferTail = (bufferTail + 1) & QUEUE_MASK;

        uint8_t outputData[CAN_LEN + 2 + 4]; //2 header bytes + 4 ID bytes + CAN_LEN data bytes

        // Write Header Bits
        outputData[0] = 0xF5;
        outputData[1] = 0xAE;

        //Write the ID Bits in big endian format
        outputData[2] = (uint8_t) ((messageID >> 24) & 0xFF);
        outputData[3] = (uint8_t) ((messageID >> 16) & 0xFF);
        outputData[4] = (uint8_t) ((messageID >> 8) & 0xFF);
        outputData[5] = (uint8_t) ((messageID >> 0) & 0xFF);

        //Sent 14 bit UART packet to the HAL 
        for(int i = 0; i < CAN_LEN; i++)
        {
            outputData[6+i] = messageData[i];
        }

        HAL_UartSend(outputData);
    }

    if(bufferSize() == 0)
    {
        //set LED to red, as there are no more elements to process
        HAL_SetLED(255, 0 ,0);
    }
}

void RxCan()  
{
    // Called every time a CAN frame is received,
    // using an interrupt. So you CANNOT call
    // any HAL_* functions from here except
    // HAL_ReadCanMessage, which is how you
    // pull a message from the bus, example 
    // usage for getting the id and data. 

    // Read in CAN data
    uint8_t data[CAN_LEN]; // 8 bytes
    uint32_t id;
    HAL_ReadCanMessage(&id, data);

    uint32_t head = bufferHead; //A volatile read, no other part of the program should change this value
    uint32_t next_head = (head + 1) & QUEUE_MASK;
    uint32_t tail = bufferTail; //Volatile read, should not change as only the interrupt will be running

    if(next_head = tail)
    {
        /* No more space in the buffer, drop message
        Testing and using a sufficiently large buffer order will prevent dropped messages */

        return;
    }

    buffer[head].id = id;
    
    for(int i = 0; i < CAN_LEN; i++)
    {
        buffer[head].data[i] = data[i];
    }

    bufferHead = next_head;
}

