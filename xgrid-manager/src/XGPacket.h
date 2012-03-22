/************************************************************************/
/* XGPacket                                                             */
/*                                                                      */
/* XGrid Packet                                                         */
/*                                                                      */
/* XGPacket.h                                                           */
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
/* MEXGHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#ifndef __XG_PACKET_H
#define __XG_PACKET_H

#include <string>
#include <vector>
#include <deque>
#include <inttypes.h>

#define XGRID_IDENTIFIER 0x5A

// XGPacket class
class XGPacket
{
public:
        // header structures
        typedef struct
        {
                uint16_t source_id;
                uint8_t type;
                uint8_t seq;
                uint8_t flags;
                uint8_t radius;
        } __attribute__ ((gcc_struct, __packed__)) xgrid_header_t;
        
        XGPacket();
        virtual ~XGPacket();
        
        // overall packet
        std::vector<uint8_t> payload;
        
        // packet fields
        uint16_t source_id;
        uint8_t type;
        uint8_t seq;
        uint8_t flags;
        uint8_t radius;
        
        std::vector<uint8_t> data;
        
        void zero();
        uint16_t get_length();
        std::vector<uint8_t> get_raw_packet();
        bool read_packet(std::vector<char> bytes, size_t &bytes_read);
        bool read_packet(std::vector<uint8_t> bytes, size_t &bytes_read);
        bool read_packet(std::deque<char> bytes, size_t &bytes_read);
        bool read_packet(std::deque<uint8_t> bytes, size_t &bytes_read);
        bool read_packet(uint8_t *bytes, size_t count, size_t &bytes_read);
        bool read_packet_bare(std::vector<char> bytes, size_t &bytes_read);
        bool read_packet_bare(std::vector<uint8_t> bytes, size_t &bytes_read);
        bool read_packet_bare(std::deque<char> bytes, size_t &bytes_read);
        bool read_packet_bare(std::deque<uint8_t> bytes, size_t &bytes_read);
        bool read_packet_bare(uint8_t *bytes, size_t count, size_t &bytes_read);
        bool build_packet();
        bool decode_packet();
        
        std::string get_hex_packet();
        
protected:
        // read and write payload data
        uint8_t read_payload_uint8(int offset);
        uint16_t read_payload_uint16(int offset);
        uint32_t read_payload_uint32(int offset);
        uint64_t read_payload_uint64(int offset);
        void write_payload_uint8(int offset, uint8_t value);
        void write_payload_uint16(int offset, uint16_t value);
        void write_payload_uint32(int offset, uint32_t value);
        void write_payload_uint64(int offset, uint64_t value);
};

#endif //__XG_PACKET_H
