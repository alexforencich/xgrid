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

#if PROGMEM_SIZE > 0x010000
#define PGM_READ_BYTE pgm_read_byte_far
#define PGM_READ_WORD pgm_read_word_far
#define PGM_READ_DWORD pgm_read_dword_far
#else
#define PGM_READ_BYTE pgm_read_byte_near
#define PGM_READ_WORD pgm_read_word_near
#define PGM_READ_DWORD pgm_read_dword_near
#endif

Xgrid::Xgrid() :
        cur_seq(0),
        timeout(0),
        state(XGRID_STATE_INIT),
        firmware_offset(0),
        firmware_updated(0),
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
        
        // calculate local id
        // simply crc of user sig row
        // likely to be unique and constant for each chip
        NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
        
        for (uint32_t i = 0x08; i <= 0x15; i++)
        {
                b = PGM_READ_BYTE(i);
                crc = _crc16_update(crc, b);
        }
        
        my_id = crc;
        
        NVM_CMD = NVM_CMD_NO_OPERATION_gc;
        
        // set initial delay
        // add some entropy
        delay = (3 * 1000) + (my_id & 0x03FF);
        
        xboot_app_crc16(&firmware_crc);
        
        build_number = (uint32_t) &__BUILD_NUMBER;
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
                nodes[node_cnt].build = 0;
                nodes[node_cnt].crc = 0;
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
        uint8_t saved_status = SREG;
        cli();
        
        // drop packet if not firmware releated during update cycle
        if (state == XGRID_STATE_FW_RX && ((pkt->type & 0xF0) != 0xF0))
        {
                SREG = saved_status;
                return;
        }
        // don't send extra packets to the node we're updating
        if (state == XGRID_STATE_FW_TX && ((pkt->type & 0xF0) != 0xF0))
                mask &= ~ update_node_mask;
        
        // get buffer index
        int8_t bi = get_free_buffer(pkt->data_len);
        
        if (bi < 0)
        {
                SREG = saved_status;
                return;
        }
        
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
        
        SREG = saved_status;
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
        
        // process nodes
        for (uint8_t i = 0; i < node_cnt; i++)
        {
                IOStream *stream = nodes[i].stream;
                
                // drop chars if necessary
                // for discarding duplicate packets
                while (nodes[i].drop_chars > 0 && stream->available())
                {
                        stream->get();
                        nodes[i].drop_chars--;
                }
                
                if (nodes[i].stream->available())
                {
                        // Process receive data
                        if (nodes[i].rx_buffer == -1)
                        {
                                uint16_t len;
                                
                                // drop chars to get to identifier
                                while (stream->available() > 0 && stream->peek() != XGRID_IDENTIFIER)
                                        stream->get();
                                
                                // continue if we're not looking at a packet
                                if (stream->peek() != XGRID_IDENTIFIER)
                                        continue;
                                
                                // grab length
                                if (stream->available() < 3)
                                        continue;
                                
                                len = stream->peek(1) | (stream->peek(2) << 8);
                                
                                int8_t bi = get_free_buffer(len-sizeof(xgrid_header_short_t));
                                
                                if (bi < 0)
                                        continue;
                                
                                nodes[i].rx_buffer = bi;
                                
                                pkt_buffer[bi].flags |= XGRID_BUFFER_IN_USE_RX;
                                pkt_buffer[bi].flags &= ~XGRID_BUFFER_UNIQUE;
                                pkt_buffer[bi].ptr = 0;
                        }
                        
                        xgrid_buffer_t *buffer = &(pkt_buffer[nodes[i].rx_buffer]);
                        
                        // read header
                        while (buffer->ptr < sizeof(xgrid_header_t) && nodes[i].stream->available())
                        {
                                ((uint8_t *)&(buffer->hdr))[buffer->ptr++] = nodes[i].stream->get();
                        }
                        
                        if (buffer->ptr < sizeof(xgrid_header_t))
                                continue;
                        
                        // grab header
                        pkt.source_id = buffer->hdr.source_id;
                        pkt.type = buffer->hdr.type;
                        pkt.seq = buffer->hdr.seq;
                        pkt.flags = buffer->hdr.flags;
                        pkt.radius = buffer->hdr.radius;
                        pkt.rx_node = i;
                        
                        // is packet unique?
                        if (!(buffer->flags & XGRID_BUFFER_UNIQUE))
                        {
                                if (pkt.type == XGRID_PKT_FLUSH_COMPARE_BUFFER ||
                                        ((pkt.type != XGRID_PKT_FIRMWARE_BLOCK || (state == XGRID_STATE_FW_RX && update_node_mask == pkt.rx_node)) &&
                                        !(state == XGRID_STATE_FW_RX && ((pkt.type & 0xF0) != 0xF0)) &&
                                        check_unique(&pkt)))
                                {
                                        buffer->flags |= XGRID_BUFFER_UNIQUE;
                                }
                                else
                                {
                                        // drop remainder
                                        nodes[i].drop_chars = (buffer->hdr.size+3) - buffer->ptr;
                                        
                                        // release buffer
                                        buffer->flags &= ~ XGRID_BUFFER_IN_USE;
                                        nodes[i].rx_buffer = -1;
                                        
                                        continue;
                                }
                        }
                        
                        // read data
                        while (buffer->ptr < buffer->hdr.size+3 && nodes[i].stream->available())
                        {
                                buffer->buffer[buffer->ptr++ - sizeof(xgrid_header_t)] = nodes[i].stream->get();
                        }
                        
                        // are we done?
                        if (buffer->ptr >= buffer->hdr.size+3)
                        {
                                // set up data reference
                                pkt.data = buffer->buffer;
                                pkt.data_len = buffer->hdr.size - sizeof(xgrid_header_short_t);
                                
                                // process packet
                                if (pkt.radius > 1)
                                {
                                        uint8_t use_current = 1;
                                        uint16_t mask = 0xFFFF;
                                        if (pkt.rx_node < 16)
                                                mask &= ~(1 << pkt.rx_node);
                                        
                                        if (buffer->hdr.flags & XGRID_PKT_FLAG_TRACE)
                                        {
                                                buffer->hdr.size++;
                                                if (buffer->hdr.size - sizeof(xgrid_header_short_t) <= buffer->buffer_len)
                                                {
                                                        buffer->buffer[buffer->hdr.size - sizeof(xgrid_header_short_t) - 1] = pkt.rx_node;
                                                }
                                                else
                                                {
                                                        // TODO
                                                }
                                        }
                                        
                                        if (use_current)
                                        {
                                                buffer->hdr.radius--;
                                                buffer->mask = mask;
                                                buffer->flags |= XGRID_BUFFER_IN_USE_TX;
                                                buffer->ptr = 0;
                                        }
                                }
                                
                                internal_process_packet(&pkt);
                                
                                // release buffer
                                buffer->flags &= ~ XGRID_BUFFER_IN_USE_RX;
                                nodes[i].rx_buffer = -1;
                        }
                }
        }
        
        // process transmit buffers
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
        }
        
        // state machine timeout
        if (timeout > 0)
        {
                timeout--;
                
                if (timeout == 0)
                {
#ifdef DEBUG
                        printf_P(PSTR("timeout!\n"));
#endif // DEBUG
                        firmware_offset = 0;
                        state = XGRID_STATE_IDLE;
                }
        }
        
        // state machine and periodic tasks
        if (delay > 0)
        {
                // delay processing
                delay--;
        }
        else if (state == XGRID_STATE_INIT)
        {
#ifdef DEBUG
                printf_P(PSTR("init, send flush command\n"));
#endif // DEBUG
                pkt.type = XGRID_PKT_FLUSH_COMPARE_BUFFER;
                pkt.flags = 0;
                pkt.radius = 1;
                pkt.data = 0;
                pkt.data_len = 0;
                
                send_packet(&pkt);
                
                delay = 1000;
                state = XGRID_STATE_IDLE;
                timeout = 0;
        }
        else if (state == XGRID_STATE_IDLE)
        {
#ifdef DEBUG
                printf_P(PSTR("idle, send version check\n"));
#endif // DEBUG
                // if we're idle, send a ping request
                // to get neighbor firmware information
                pkt.type = XGRID_PKT_PING_REQUEST;
                pkt.flags = 0;
                pkt.radius = 1;
                pkt.data_len = 0;
                
                send_packet(&pkt);
                
                // wait 100 cycles
                delay = 100;
                state = XGRID_STATE_CHECK_VER;
                timeout = 0;
        }
        else if (state == XGRID_STATE_CHECK_VER)
        {
#ifdef DEBUG
                printf_P(PSTR("check neighbor versions\n"));
#endif // DEBUG
                // check versions every 30 seconds
                // at 1 kHz tick rate
                delay = 30*1000;
                state = XGRID_STATE_IDLE;
                
                update_node_mask = 0;
                
#ifdef DEBUG
                printf_P(PSTR("update check %ld\n"), build_number);
#endif // DEBUG
                
                // check detected revisions
                for (uint8_t n = 0; n < node_cnt; n++)
                {
#ifdef DEBUG
                        printf_P(PSTR("node %d: %ld (%04x)\n"), n, nodes[n].build, nodes[n].crc);
#endif // DEBUG
                        if (nodes[n].build > 0 && nodes[n].build < build_number)
                        {
                                update_node_mask |= (1 << n);
                        }
                        else if (nodes[n].build > build_number)
                        {
                                // we're old, don't try to update anybody
                                update_node_mask = 0;
                                break;
                        }
                }
                
                // need to update somebody?
                if (update_node_mask != 0)
                {
#ifdef DEBUG
                        printf_P(PSTR("send start update command\n"));
#endif // DEBUG
                        // send start update command
                        uint8_t buffer[11];
                        //xgrid_pkt_maint_cmd_t *c = (xgrid_pkt_maint_cmd_t *)buffer;
                        xgrid_pkt_maint_cmd_start_update_t *c = (xgrid_pkt_maint_cmd_start_update_t *)buffer;
                        pkt.type = XGRID_PKT_MAINT_CMD;
                        pkt.flags = 0;
                        pkt.radius = 1;
                        pkt.data = buffer;
                        pkt.data_len = sizeof(xgrid_pkt_maint_cmd_start_update_t);
                        
                        c->cmd = XGRID_CMD_START_UPDATE;
                        
                        c->magic = XGRID_CMD_UPDATE_MAGIC;
                        
                        c->crc = firmware_crc;
                        c->build = build_number;
                        
                        send_packet(&pkt, update_node_mask);
                        
                        if (firmware_updated)
                                firmware_offset = XB_APP_TEMP_START;
                        else
                                firmware_offset = 0;
                        
                        // start sending new firmware after small delay
                        delay = 100;
                        state = XGRID_STATE_FW_TX;
                }
                else
                {
                        // if we've been updated and no one else needs an update,
                        // install the firmware and reset
                        if (firmware_updated)
                        {
                                xboot_reset();
                        }
                }
        }
        else if (state == XGRID_STATE_FW_TX)
        {
                if (firmware_offset < XB_APP_SIZE ||
                        (firmware_updated && firmware_offset < (XB_APP_TEMP_START + XB_APP_TEMP_SIZE)))
                {
#ifdef DEBUG
                        printf_P(PSTR("send firmware block\n"));
#endif // DEBUG
                        // send flash block
                        
                        uint8_t buffer[SPM_PAGESIZE+2];
                        xgrid_pkt_firmware_block_t *b = (xgrid_pkt_firmware_block_t *)buffer;
                        pkt.type = XGRID_PKT_FIRMWARE_BLOCK;
                        pkt.flags = 0;
                        pkt.radius = 1;
                        pkt.data = buffer;
                        pkt.data_len = SPM_PAGESIZE+2;
                        
                        if (firmware_updated)
                                b->offset = (firmware_offset - XB_APP_TEMP_START)/SPM_PAGESIZE;
                        else
                                b->offset = firmware_offset/SPM_PAGESIZE;
                        
                        for (uint16_t i = 0; i < SPM_PAGESIZE; i++)
                        {
                                b->data[i] = PGM_READ_BYTE(firmware_offset++);
                        }
                        
                        if (firmware_offset == XB_APP_TEMP_END)
                        {
                                for (uint16_t i = SPM_PAGESIZE - 7; i < SPM_PAGESIZE; i++)
                                        b->data[i] = 0xff;
                        }
                        
                        send_packet(&pkt, update_node_mask);
                        
                        // small delay before next packet
                        // long enough to clear data through serial
                        // port buffer and write the block at the other end
                        delay = 100;
                }
                else
                {
#ifdef DEBUG
                        printf_P(PSTR("finished sending firmware\n"));
#endif // DEBUG
                        // send finish update command
                        uint8_t buffer[5];
                        xgrid_pkt_maint_cmd_t *c = (xgrid_pkt_maint_cmd_t *)buffer;
                        pkt.type = XGRID_PKT_MAINT_CMD;
                        pkt.flags = 0;
                        pkt.radius = 1;
                        pkt.data = buffer;
                        pkt.data_len = 5;
                        
                        c->cmd = XGRID_CMD_FINISH_UPDATE;
                        
                        c->magic = XGRID_CMD_UPDATE_MAGIC;
                        
                        send_packet(&pkt, update_node_mask);
                        
                        // flush old build information
                        for (int i = 0; i < node_cnt; i++)
                        {
                                nodes[i].build = 0;
                                nodes[i].crc = 0;
                        }
                        
                        // check again after short delay
                        delay = 1000;
                        state = XGRID_STATE_IDLE;
                }
        }
        else if (state == XGRID_STATE_FW_RX)
        {
                // nothing special
        }
        else
        {
                // invalid state, go back to idle, do not collect $200
                state = XGRID_STATE_IDLE;
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
                
                internal_process_packet(pkt);
        }
}


void Xgrid::internal_process_packet(Packet *pkt)
{
        if (pkt->type == XGRID_PKT_PING_REQUEST)
        {
#ifdef DEBUG
                printf_P(PSTR("rx ping req\n"));
#endif // DEBUG
                xgrid_pkt_ping_reply_t d;
                d.build = build_number;
                d.crc = firmware_crc;
                
                Packet reply;
                reply.type = XGRID_PKT_PING_REPLY;
                reply.flags = 0;
                reply.radius = 1;
                reply.data = (uint8_t *)&d;
                reply.data_len = sizeof(xgrid_pkt_ping_reply_t);
                
                send_packet(&reply, (1 << pkt->rx_node));
        }
        else if (pkt->type == XGRID_PKT_PING_REPLY)
        {
#ifdef DEBUG
                printf_P(PSTR("rx ping reply\n"));
#endif // DEBUG
                xgrid_pkt_ping_reply_t *d = (xgrid_pkt_ping_reply_t *)(pkt->data);
                
                nodes[pkt->rx_node].build = d->build;
                nodes[pkt->rx_node].crc = d->crc;
        }
        else if (pkt->type == XGRID_PKT_MAINT_CMD)
        {
#ifdef DEBUG
                printf_P(PSTR("rx maint cmd\n"));
#endif // DEBUG
                xgrid_pkt_maint_cmd_t *c = (xgrid_pkt_maint_cmd_t *)(pkt->data);
                
                if (c->cmd == XGRID_CMD_START_UPDATE && c->magic == XGRID_CMD_UPDATE_MAGIC &&
                                state != XGRID_STATE_FW_RX)
                {
#ifdef DEBUG
                        printf_P(PSTR("start update\n"));
#endif // DEBUG
                        // start update
                        firmware_offset = 0;
                        
                        xgrid_pkt_maint_cmd_start_update_t *csu = (xgrid_pkt_maint_cmd_start_update_t *)(pkt->data);
                        
                        if (csu->build > build_number)
                        {
                                // sanity check: only update if being
                                // offered a more recent version
                                
                                new_build = csu->build;
                                new_crc = csu->crc;
                                
                                update_node_mask = pkt->rx_node;
                                state = XGRID_STATE_FW_RX;
                                timeout = 1000;
                        }
                }
                else if (c->cmd == XGRID_CMD_FINISH_UPDATE && c->magic == XGRID_CMD_UPDATE_MAGIC &&
                                state == XGRID_STATE_FW_RX)
                {
#ifdef DEBUG
                        printf_P(PSTR("finish update\n"));
#endif // DEBUG
                        // check and install firmware
                        
                        uint16_t cur_crc;
                        
                        xboot_app_temp_crc16(&cur_crc);
                        
#ifdef DEBUG
                        printf_P(PSTR("new crc: %04x\n"), new_crc);
                        printf_P(PSTR("cur crc: %04x\n"), cur_crc);
#endif // DEBUG
                        
                        if (cur_crc == new_crc)
                        {
#ifdef DEBUG
                                printf_P(PSTR("good crc\n"));
#endif // DEBUG
                                firmware_crc = new_crc;
                                build_number = new_build;
                                firmware_offset = XB_APP_TEMP_START;
                                firmware_updated = 1;
                                // init install
                                xboot_install_firmware(new_crc);
                        }
                        else
                        {
#ifdef DEBUG
                                printf_P(PSTR("bad crc\n"));
#endif // DEBUG
                                firmware_offset = 0;
                        }
                        
                        // go back to idle and check neighbor firmware versions
                        timeout = 0;
                        delay = 100;
                        state = XGRID_STATE_IDLE;
                }
                else if (c->cmd == XGRID_CMD_ABORT_UPDATE && c->magic == XGRID_CMD_UPDATE_MAGIC &&
                                state == XGRID_STATE_FW_RX)
                {
#ifdef DEBUG
                        printf_P(PSTR("abort update\n"));
#endif // DEBUG
                        // abort update (go back to idle)
                        firmware_offset = 0;
                        state = XGRID_STATE_IDLE;
                }
                else if (c->cmd == XGRID_CMD_RESET && c->magic == XGRID_CMD_RESET_MAGIC)
                {
#ifdef DEBUG
                        printf_P(PSTR("reset\n"));
#endif // DEBUG
                        xboot_reset();
                }
        }
        else if (pkt->type == XGRID_PKT_FIRMWARE_BLOCK)
        {
#ifdef DEBUG
                printf_P(PSTR("rx firmware block\n"));
#endif // DEBUG
                if (state == XGRID_STATE_FW_RX && pkt->data_len == SPM_PAGESIZE+2)
                {
                        xgrid_pkt_firmware_block_t *b = (xgrid_pkt_firmware_block_t *)(pkt->data);
                        
                        xboot_app_temp_write_page(b->offset * SPM_PAGESIZE, b->data, 1);
                        
                        timeout = 1000;
                }
        }
        else if (pkt->type == XGRID_PKT_FLUSH_COMPARE_BUFFER)
        {
#ifdef DEBUG
                printf_P(PSTR("rx flush buffer\n"));
#endif // DEBUG
                memset(compare_buffer, 0, sizeof(xgrid_header_minimal_t) * XGRID_COMPARE_BUFFER_SIZE);
        }
        else
        {
                // if we haven't processed the packet internally,
                // pass it to the application
                if (rx_pkt)
                        (*rx_pkt)(pkt);
        }
}


