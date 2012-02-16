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

// defines
#define XGRID_MAX_NODES 8
#define XGRID_COMPARE_BUFFER_SIZE 16
#define XGRID_BUFFER_SIZE 64

#define XGRID_IDENTIFIER 0x5A
#define XGRID_ESCAPE 0x55

// packet types


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
        
        // Per object data
        uint16_t my_id;
        uint8_t cur_seq;
        
        IOStream *nodes[XGRID_MAX_NODES];
        int8_t node_cnt;
        
        xgrid_header_minimal_t compare_buffer[XGRID_COMPARE_BUFFER_SIZE];
        int8_t compare_buffer_ptr;
        
        // Static data
        
        // Private methods
        void populate_packet(Packet *pkt, uint8_t *buffer);
        uint8_t is_unique(Packet *pkt);
        uint8_t check_unique(Packet *pkt);
        
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



