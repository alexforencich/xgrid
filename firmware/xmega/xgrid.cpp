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
        
        // init packet buffers
        for (int i = 0; i < XGRID_SM_BUFFER_COUNT; i++)
        {
                pkt_buffer[i].buffer = pkt_buffer_sm[i];
                pkt_buffer[i].buffer_len = XGRID_SM_BUFFER_SIZE;
                pkt_buffer[i].flags = 0;
        }
        
        for (int i = 0; i < XGRID_LG_BUFFER_COUNT; i++)
        {
                pkt_buffer[XGRID_SM_BUFFER_COUNT+i].buffer = pkt_buffer_lg[i];
                pkt_buffer[XGRID_SM_BUFFER_COUNT+i].buffer_len = XGRID_LG_BUFFER_SIZE;
                pkt_buffer[XGRID_SM_BUFFER_COUNT+i].flags = 0;
        }
        
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
                nodes[node_cnt].stream = stream;
                nodes[node_cnt].tx_buffer = -1;
                nodes[node_cnt].rx_buffer = -1;
                nodes[node_cnt].drop_chars = 0;
                return node_cnt++;
        }
        
        return -1;
}


void Xgrid::populate_packet(Packet *pkt, uint8_t *buffer)
{
        xgrid_header_t *hdr = (xgrid_header_t *)buffer;
        
        hdr->identifier = XGRID_IDENTIFIER;
        hdr->size = pkt->data_len + sizeof(xgrid_header_short_t);
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


int8_t Xgrid::get_free_buffer(uint16_t data_size)
{
        for (int i = 0; i < XGRID_BUFFER_COUNT; i++)
        {
                if ((pkt_buffer[i].flags & XGRID_BUFFER_IN_USE) == 0 && pkt_buffer[i].buffer_len >= data_size)
                        return i;
        }
        return -1;
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
        // get buffer index
        int8_t bi = get_free_buffer(pkt->data_len);
        
        if (bi < 0)
                return;
        
        xgrid_buffer_t *buffer = &(pkt_buffer[bi]);
        
        // flag it for use
        buffer->flags |= XGRID_BUFFER_IN_USE_TX;
        
        xgrid_header_t *hdr = &(buffer->hdr);
        
        // packet header information
        hdr->identifier = XGRID_IDENTIFIER;
        hdr->size = pkt->data_len + sizeof(xgrid_header_short_t);
        hdr->source_id = pkt->source_id;
        hdr->type = pkt->type;
        hdr->seq = pkt->seq;
        hdr->flags = pkt->flags;
        hdr->radius = pkt->radius;
        
        buffer->mask = mask;
        
        // copy in data
        for (uint16_t i = 0; i < pkt->data_len; i++)
        {
                buffer->buffer[i] = pkt->data[i];
        }
        
        // start at zero
        buffer->ptr = 0;
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
        
        // process nodes
        for (uint8_t i = 0; i < node_cnt; i++)
        {
                // drop chars if necessary
                // for discarding duplicate packets
                while (nodes[i].drop_chars > 0 && nodes[i].stream->available())
                {
                        nodes[i].stream->get();
                        nodes[i].drop_chars--;
                }
                
                pkt.data = buffer;
                ret = try_read_packet(&pkt, nodes[i].stream);
                
                if (ret)
                {
                        pkt.rx_node = i;
                        process_packet(&pkt);
                }
        }
        
        // process buffers
        for (uint8_t i = 0; i < XGRID_BUFFER_COUNT; i++)
        {
                xgrid_buffer_t *buffer = &(pkt_buffer[i]);
                
                if (buffer->flags & XGRID_BUFFER_IN_USE_TX)
                {
                        // process for transmit buffer
                        
                        // find minimum free
                        uint16_t f = 0xffff;
                        
                        for (uint8_t n = 0; n < node_cnt; n++)
                        {
                                if (buffer->mask & (1 << n))
                                {
                                        // check node buffer assignment
                                        if (nodes[n].tx_buffer == -1)
                                        {
                                                // if not assigned, set
                                                nodes[n].tx_buffer = i;
                                        }
                                        else if (nodes[n].tx_buffer != i)
                                        {
                                                // if assigned to different buffer, hold packet
                                                f = 0;
                                        }
                                        
                                        uint16_t f2 = nodes[n].stream->free();
                                        if (f > f2)
                                                f = f2;
                                }
                        }
                        
                        // send as much of packet as possible
                        for (uint8_t n = 0; n < node_cnt; n++)
                        {
                                if (buffer->mask & (1 << n))
                                {
                                        uint16_t cnt = f;
                                        uint16_t ptr = buffer->ptr;
                                        
                                        // header
                                        while (cnt > 0 && ptr < sizeof(xgrid_header_t))
                                        {
                                                nodes[n].stream->put(((uint8_t *)&(buffer->hdr))[ptr]);
                                                ptr++;
                                                cnt--;
                                        }
                                        
                                        // data
                                        while (cnt > 0 && ptr < buffer->hdr.size+3)
                                        {
                                                nodes[n].stream->put(buffer->buffer[ptr-sizeof(xgrid_header_t)]);
                                                ptr++;
                                                cnt--;
                                        }
                                        
                                }
                        }
                        
                        buffer->ptr += f;
                        
                        // are we done?
                        if (buffer->ptr >= buffer->hdr.size+3)
                        {
                                // turn off flag
                                buffer->flags &= ~XGRID_BUFFER_IN_USE;
                                
                                // remove buffer assigments
                                for (uint8_t n = 0; n < node_cnt; n++)
                                {
                                        if (buffer->mask & (1 << n))
                                        {
                                                nodes[n].tx_buffer = -1;
                                        }
                                }
                        }
                }
                else if (buffer->flags & XGRID_BUFFER_IN_USE_RX)
                {
                        // TODO
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


