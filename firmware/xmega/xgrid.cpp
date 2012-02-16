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

#include "xgrid.h"

#include <avr/pgmspace.h>
#include <util/crc16.h>
#include <string.h>

Xgrid::Xgrid() :
        cur_seq(0),
        node_cnt(0),
        compare_buffer_ptr(0),
        rx_pkt(0)
{
        uint8_t b;
        uint16_t crc = 0;
        
        // zero compare buffer
        memset(compare_buffer, 0, sizeof(xgrid_header_minimal_t) * XGRID_COMPARE_BUFFER_SIZE);
        
        NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
        
        for (uint32_t i = 0x08; i <= 0x15; i++)
        {
                b = pgm_read_byte(i);
                crc = _crc16_update(crc, b);
        }
        
        my_id = crc;
        
        NVM_CMD = NVM_CMD_NO_OPERATION_gc;
}


Xgrid::~Xgrid()
{
        
}


uint16_t Xgrid::get_id()
{
        return my_id;
}


int8_t Xgrid::add_node(IOStream *stream)
{
        if (node_cnt < XGRID_MAX_NODES)
        {
                nodes[node_cnt] = stream;
                return node_cnt++;
        }
        
        return -1;
}


void Xgrid::populate_packet(Packet *pkt, uint8_t *buffer)
{
        xgrid_header_t *hdr = (xgrid_header_t *)buffer;
        
        hdr->identifier = XGRID_IDENTIFIER;
        hdr->size = pkt->data_len + 6;
        hdr->source_id = pkt->source_id;
        hdr->type = pkt->type;
        hdr->seq = pkt->seq;
        hdr->flags = pkt->flags;
        hdr->radius = pkt->radius;
}


uint8_t Xgrid::is_unique(Packet *pkt)
{
        for (uint8_t i = 0; i < XGRID_COMPARE_BUFFER_SIZE; i++)
        {
                if (compare_buffer[i].seq == pkt->seq &&
                        compare_buffer[i].source_id == pkt->source_id &&
                        compare_buffer[i].type == pkt->type)
                        
                        return 0;
        }
        
        return 1;
}


uint8_t Xgrid::check_unique(Packet *pkt)
{
        uint8_t ret = is_unique(pkt);
        
        if (ret)
        {
                compare_buffer[compare_buffer_ptr].source_id = pkt->source_id;
                compare_buffer[compare_buffer_ptr].type = pkt->type;
                compare_buffer[compare_buffer_ptr].seq = pkt->seq;
                
                compare_buffer_ptr++;
                if (compare_buffer_ptr >= XGRID_COMPARE_BUFFER_SIZE)
                        compare_buffer_ptr = 0;
        }
        
        return ret;
}


void Xgrid::send_packet(Packet *pkt, uint16_t mask)
{
        pkt->source_id = my_id;
        pkt->seq = cur_seq++;
        pkt->rx_node = 0xFF;
        
        send_raw_packet(pkt, mask);
}


void Xgrid::send_raw_packet(Packet *pkt, uint16_t mask)
{
        uint8_t buffer[XGRID_BUFFER_SIZE];
        uint16_t len = pkt->data_len;
        
        populate_packet(pkt, buffer);
        
        for (uint16_t i = 0; i < len; i++)
        {
                buffer[i + sizeof(xgrid_header_t)] = pkt->data[i];
        }
        
        len += sizeof(xgrid_header_t);
        
        for (uint8_t i = 0; i < node_cnt; i++)
        {
                if (mask & (1 << i))
                {
                        nodes[i]->write(buffer, len);
                }
        }
}


uint8_t Xgrid::try_read_packet(Packet *pkt, IStream *stream)
{
        uint8_t buffer[XGRID_BUFFER_SIZE];
        uint16_t len;
        
        // don't do anything if data pointer is null
        if (pkt->data == 0)
                return 0;
        
        // drop bytes until we get a XGRID_IDENTIFIER
        while (stream->available() > 0 && stream->peek() != XGRID_IDENTIFIER)
                stream->get();
        
        // return if we're not looking at a packet
        if (stream->peek() != XGRID_IDENTIFIER)
                return 0;
        
        // grab length
        if (stream->available() < 3)
                return 0;
        
        len = stream->peek(1) | (stream->peek(2) << 8);
        
        // return if the whole packet isn't available yet
        if (stream->available() < len + 3)
                return 0;
        
        // drop header
        stream->get();
        stream->get();
        stream->get();
        
        // read data
        stream->read(buffer, len);
        
        return try_parse_packet(pkt, buffer, len);
}


uint8_t Xgrid::try_parse_packet(Packet *pkt, const uint8_t *buffer, uint16_t len)
{
        if (len < sizeof(xgrid_header_short_t))
                return 0;
        
        xgrid_header_short_t *hdr = (xgrid_header_short_t *)buffer;
        
        pkt->source_id = hdr->source_id;
        pkt->type = hdr->type;
        pkt->seq = hdr->seq;
        pkt->flags = hdr->flags;
        pkt->radius = hdr->radius;
        
        len -= sizeof(xgrid_header_short_t);
        
        for (uint16_t i = 0; i < len; i++)
        {
                pkt->data[i] = buffer[i + sizeof(xgrid_header_short_t)];
        }
        
        pkt->data_len = len;
        
        return 1;
}


void Xgrid::process()
{
        Packet pkt;
        uint8_t buffer[XGRID_BUFFER_SIZE];
        
        uint8_t ret;
        
        for (uint8_t i = 0; i < node_cnt; i++)
        {
                pkt.data = buffer;
                ret = try_read_packet(&pkt, nodes[i]);
                
                if (ret)
                {
                        pkt.rx_node = i;
                        process_packet(&pkt);
                }
        }
}


void Xgrid::process_packet(Packet *pkt)
{
        if (check_unique(pkt))
        {
                if (pkt->radius > 1)
                {
                        pkt->radius--;
                        uint16_t mask = 0xFFFF;
                        if (pkt->rx_node < 16)
                                mask &= ~(1 << pkt->rx_node);
                        send_raw_packet(pkt, mask);
                        pkt->radius++;
                }
                
                if (rx_pkt)
                        (*rx_pkt)(pkt);
        }
}


