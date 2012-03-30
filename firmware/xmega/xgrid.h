/************************************************************************/
/* xgrid                                                                */
/*                                                                      */
/* xgrid.cpp                                                            */
/*                                                                      */
/* Alex Forencich <alex@alexforencich.com>                              */
/*                                                                      */
/* Copyright (c) 2011 Alex Forencich                                    */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation       */
/* files(the "Software"), to deal in the Software without restriction,  */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies or substantial portions of the Software.      */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#ifndef __XGRID_H
#define __XGRID_H

#include <avr/io.h>
#include <avr/interrupt.h>

#include "iostream.h"
#include "../xboot/xbootapi.h"

// Build information
extern char   __BUILD_DATE;
extern char   __BUILD_NUMBER;

// defines
#define XGRID_MAX_NODES 8
#define XGRID_COMPARE_BUFFER_SIZE 16
#define XGRID_BUFFER_SIZE 64

#define XGRID_SM_BUFFER_COUNT   10
#define XGRID_SM_BUFFER_SIZE    64
#define XGRID_LG_BUFFER_COUNT   1
#define XGRID_LG_BUFFER_SIZE    (512+2)
#define XGRID_BUFFER_COUNT      (XGRID_SM_BUFFER_COUNT + XGRID_LG_BUFFER_COUNT)

#define XGRID_BUFFER_IN_USE     0x03
#define XGRID_BUFFER_IN_USE_TX  0x01
#define XGRID_BUFFER_IN_USE_RX  0x02
#define XGRID_BUFFER_UNIQUE     0x04

#define XGRID_IDENTIFIER 0x5A
#define XGRID_ESCAPE 0x55

// packet types
#include "xgrid_types.h"

// flags
#define XGRID_PKT_FLAG_TRACE    0x10

// states
#define XGRID_STATE_IDLE        0x00
#define XGRID_STATE_INIT        0x01
#define XGRID_STATE_CHECK_VER   0x10
#define XGRID_STATE_FW_TX       0x20
#define XGRID_STATE_FW_RX       0x28

#define DEBUG

// Xgrid class
class Xgrid
{
public:
        // typedefs
        typedef struct
        {
                // Packet parameters
                uint16_t source_id;
                uint8_t type;
                uint8_t seq;
                uint8_t flags;
                uint8_t radius;
                // data
                uint8_t *data;
                uint16_t data_len;
                // metadata
                uint8_t rx_node;
                uint8_t m_flags;
        }
        Packet;
        
private:
        // Private typedefs
        typedef struct
        {
                uint8_t identifier;
                uint16_t size;
                uint16_t source_id;
                uint8_t type;
                uint8_t seq;
                uint8_t flags;
                uint8_t radius;
        } __attribute__ ((__packed__)) xgrid_header_t;
        
        typedef struct
        {
                uint16_t source_id;
                uint8_t type;
                uint8_t seq;
                uint8_t flags;
                uint8_t radius;
        } __attribute__ ((__packed__)) xgrid_header_short_t;
        
        typedef struct
        {
                uint16_t source_id;
                uint8_t type;
                uint8_t seq;
        } __attribute__ ((__packed__)) xgrid_header_minimal_t;
        
        typedef struct
        {
                IOStream *stream;
                int8_t rx_buffer;
                int8_t tx_buffer;
                uint16_t drop_chars;
                uint32_t build;
                uint16_t crc;
        } xgrid_node_t;
        
        typedef struct
        {
                xgrid_header_t hdr;
                uint8_t *buffer;
                uint16_t buffer_len;
                uint16_t ptr;
                uint16_t mask;
                uint8_t flags;
        } xgrid_buffer_t;
        
        // Per object data
        uint16_t my_id;
        uint8_t cur_seq;
        
        uint16_t firmware_crc;
        uint32_t build_number;
        
        uint16_t timeout;
        uint32_t delay;
        uint8_t state;
        
        uint16_t update_node_mask;
        uint16_t new_crc;
        uint32_t new_build;
        uint32_t firmware_offset;
        uint8_t firmware_updated;
        
        // node list
        xgrid_node_t nodes[XGRID_MAX_NODES];
        int8_t node_cnt;
        
        // compare buffer
        xgrid_header_minimal_t compare_buffer[XGRID_COMPARE_BUFFER_SIZE];
        int8_t compare_buffer_ptr;
        
        // transmit and receive packet buffers
        uint8_t pkt_buffer_sm[XGRID_SM_BUFFER_COUNT][XGRID_SM_BUFFER_SIZE];
        uint8_t pkt_buffer_lg[XGRID_LG_BUFFER_COUNT][XGRID_LG_BUFFER_SIZE];
        xgrid_buffer_t pkt_buffer[XGRID_BUFFER_COUNT];
        
        // Static data
        
        // Private methods
        void populate_packet(Packet *pkt, uint8_t *buffer);
        uint8_t is_unique(Packet *pkt);
        uint8_t check_unique(Packet *pkt);
        int8_t get_free_buffer(uint16_t data_size);
        
        void internal_process_packet(Packet *pkt);
        
        // Private static methods
        
public:
        // Public variables
        
        // receive packet callback
        void (*rx_pkt)(Packet *pkt);
        
        // Public methods
        Xgrid();
        ~Xgrid();
        
        uint16_t get_id();
        
        int8_t add_node(IOStream *stream);
        
        void send_packet(Packet *pkt, uint16_t mask = 0xFFFF);
        void send_raw_packet(Packet *pkt, uint16_t mask = 0xFFFF);
        uint8_t try_read_packet(Packet *pkt, IStream *stream);
        uint8_t try_parse_packet(Packet *pkt, const uint8_t *buffer, uint16_t len);
        
        void process();
        void process_packet(Packet *pkt);
};

// Prototypes


#endif // __XGRID_H



