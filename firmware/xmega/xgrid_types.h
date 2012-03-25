/************************************************************************/
/* XGrid Packet Types                                                   */
/*                                                                      */
/* xgrid_types.cpp                                                      */
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

#ifndef __XGRID_TYPES_H
#define __XGRID_TYPES_H

#include <inttypes.h>

// packed definition
// needed as gcc_struct necessary in windows
// but causes warning in avr-gcc
#ifdef _WIN32
#define PACKED_ATTR gcc_struct, __packed__
#else
#define PACKED_ATTR __packed__
#endif

// packet types
// general purpose
#define XGRID_PKT_DEBUG 0xFF

// network
// ping packet for testing connectivity
// reply will contain firmware information
#define XGRID_PKT_PING_REQUEST 0xFD
#define XGRID_PKT_PING_REPLY 0xFE

typedef struct
{
        int32_t build;
        uint16_t crc;
} __attribute__ ((PACKED_ATTR)) xgrid_pkt_ping_reply_t;

// firmware block
// first two bytes base address / SPM_PAGESIZE
// followed by SPM_PAGESIZE bytes of firmware
#define XGRID_PKT_FIRMWARE_BLOCK 0xFB

typedef struct
{
        int16_t offset;
        uint8_t data[];
} __attribute__ ((PACKED_ATTR)) xgrid_pkt_firmware_block_t;

// reset command
// must contain magic number
#define XGRID_PKT_RESET 0xFC
#define XGRID_PKT_RESET_MAGIC 0xFEE1DEAD

#endif // __XGRID_TYPES_H


