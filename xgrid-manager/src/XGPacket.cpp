/************************************************************************/
/* XGPacket                                                             */
/*                                                                      */
/* XGrid Packet                                                         */
/*                                                                      */
/* XGPacket.cpp                                                         */
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

#include "XGPacket.h"

#include <sstream>
#include <iomanip>

XGPacket::XGPacket()
{
        zero();
}

XGPacket::~XGPacket()
{
        // nothing
}

void XGPacket::zero()
{
        source_id = 0;
        type = 0;
        seq = 0;
        flags = 0;
        radius = 0;
        data.clear();
}

uint16_t XGPacket::get_length()
{
        return data.size()+sizeof(xgrid_header_t);
}

std::vector<uint8_t> XGPacket::get_raw_packet()
{
        std::vector<uint8_t> dataout;
        
        dataout.push_back(XGRID_IDENTIFIER);
        dataout.push_back(payload.size());
        dataout.push_back(payload.size() >> 8);
        
        dataout.insert(dataout.end(), payload.begin(), payload.end());
        
        return dataout;
}

bool XGPacket::read_packet(std::vector<char> bytes, size_t &bytes_read)
{
        return read_packet((uint8_t *)&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet(std::vector<uint8_t> bytes, size_t &bytes_read)
{
        return read_packet(&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet(std::deque<char> bytes, size_t &bytes_read)
{
        return read_packet((uint8_t *)&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet(std::deque<uint8_t> bytes, size_t &bytes_read)
{
        return read_packet(&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet(uint8_t *bytes, size_t count, size_t &bytes_read)
{
        size_t n = 0;
        uint16_t size;
        uint8_t *ptr = bytes;
        uint8_t b;
        
        if (count == 0)
                return false;
        
        // find packet start byte
        while ((*(ptr++) != XGRID_IDENTIFIER) & (n++ < count)) { }
        
        // start of packet offset
        // to discard junk ahead of incomplete packet
        // (assuming packet is currently truncated)
        bytes_read = n-1;
        
        // return if out of buffer
        if (n > count)
                return false;
        
        // return if not enough bytes left
        if (count - n < sizeof(xgrid_header_t) + 2)
                return false;
        
        // get packet size
        size = (uint16_t)*(ptr++);
        size |= (uint16_t)*(ptr++) << 8;
        n += 2;
        
        // return if we don't have the whole packet
        if (count - n < size)
                return false;
        
        payload.clear();
        
        // read payload
        for (int i = 0; i < size; i++)
        {
                b = *(ptr++);
                payload.push_back(b);
                n++;
        }
        
        bytes_read = n;
        
        return true;
}

bool XGPacket::read_packet_bare(std::vector<char> bytes, size_t &bytes_read)
{
        return read_packet_bare((uint8_t *)&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet_bare(std::vector<uint8_t> bytes, size_t &bytes_read)
{
        return read_packet_bare(&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet_bare(std::deque<char> bytes, size_t &bytes_read)
{
        return read_packet_bare((uint8_t *)&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet_bare(std::deque<uint8_t> bytes, size_t &bytes_read)
{
        return read_packet_bare(&bytes[0], bytes.size(), bytes_read);
}

bool XGPacket::read_packet_bare(uint8_t *bytes, size_t count, size_t &bytes_read)
{
        size_t n = 0;
        uint8_t *ptr = bytes;
        uint8_t b;
        
        if (count < sizeof(xgrid_header_t))
                return false;
        
        payload.clear();
        
        // read payload
        for (int i = 0; i < count; i++)
        {
                b = *(ptr++);
                payload.push_back(b);
        }
        
        bytes_read = count;
        
        return true;
}

bool XGPacket::build_packet()
{
        payload.clear();
        payload.push_back(source_id);
        payload.push_back(source_id << 8);
        payload.push_back(type);
        payload.push_back(seq);
        payload.push_back(flags);
        payload.push_back(radius);
        
        for (int i = 0; i < data.size(); i++)
        {
                payload.push_back(data[i]);
        }
        
        return true;
}

bool XGPacket::decode_packet()
{
        if (payload.size() < sizeof(xgrid_header_t))
                return false;
        
        source_id = read_payload_uint16(0);
        type = payload[2];
        seq = payload[3];
        flags = payload[4];
        radius = payload[5];
        
        data.clear();
        for (int i = sizeof(xgrid_header_t); i < payload.size(); i++)
        {
                data.push_back(payload[i]);
        }
        
        return true;
}

std::string XGPacket::get_desc()
{
        std::stringstream desc;
        
        desc << "XGrid Packet" << std::endl;
        desc << "  Length: " << std::dec << get_length() << std::endl;
        desc << "  Source ID: 0x" << std::setfill('0') << std::setw(4) << std::hex << (int)source_id << std::endl;
        desc << "  Type: 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)type << std::endl;
        desc << "  Seq: " << std::dec << (int)seq << std::endl;
        desc << "  Flags: 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)flags << std::endl;
        desc << "  Radius: " << std::dec << (int)radius << std::endl;
        
        desc << "  Data (hex):";
        for (int i = 0; i < data.size(); i++)
        {
                if (i > 0 && i % 16 == 0)
                        desc << std::endl << "             ";
                desc << " " << std::setfill('0') << std::setw(2) << std::hex << (int)data[i];
        }
        desc << std::endl;
        
        return desc.str();
}

std::string XGPacket::get_hex_packet()
{
        std::vector<uint8_t> pkt = get_raw_packet();
        std::stringstream out;
        
        for (int i = 0; i < pkt.size(); i++)
        {
                if (i > 0)
                        out << " ";
                out << std::setfill('0') << std::setw(2) << std::hex << (int)pkt[i];
        }
        
        return out.str();
}


// read and write payload data
uint8_t XGPacket::read_payload_uint8(int offset)
{
        if (offset < 0)
                return 0;
        return payload[offset];
}

uint16_t XGPacket::read_payload_uint16(int offset)
{
        if (offset < 0)
                return 0;
        uint16_t value;
        value  = (uint16_t)payload[offset];
        value |= (uint16_t)payload[offset+1] << 8;
        return value;
}

uint32_t XGPacket::read_payload_uint32(int offset)
{
        if (offset < 0)
                return 0;
        uint32_t value;
        value  = (uint32_t)payload[offset];
        value |= (uint32_t)payload[offset+1] << 8;
        value |= (uint32_t)payload[offset+2] << 16;
        value |= (uint32_t)payload[offset+3] << 24;
        return value;
}

uint64_t XGPacket::read_payload_uint64(int offset)
{
        if (offset < 0)
                return 0;
        uint64_t value;
        value  = (uint64_t)payload[offset];
        value |= (uint64_t)payload[offset+1] << 8;
        value |= (uint64_t)payload[offset+2] << 16;
        value |= (uint64_t)payload[offset+3] << 24;
        value |= (uint64_t)payload[offset+4] << 32;
        value |= (uint64_t)payload[offset+5] << 40;
        value |= (uint64_t)payload[offset+6] << 48;
        value |= (uint64_t)payload[offset+7] << 56;
        return value;
}

void XGPacket::write_payload_uint8(int offset, uint8_t value)
{
        if (offset < 0)
                return;
        payload[offset] = value;
}

void XGPacket::write_payload_uint16(int offset, uint16_t value)
{
        if (offset < 0)
                return;
        payload[offset]   = value;
        payload[offset+1] = value >> 8;
}

void XGPacket::write_payload_uint32(int offset, uint32_t value)
{
        if (offset < 0)
                return;
        payload[offset]   = value;
        payload[offset+1] = value >> 8;
        payload[offset+2] = value >> 16;
        payload[offset+3] = value >> 24;
}

void XGPacket::write_payload_uint64(int offset, uint64_t value)
{
        if (offset < 0)
                return;
        payload[offset]   = value;
        payload[offset+1] = value >> 8;
        payload[offset+2] = value >> 16;
        payload[offset+3] = value >> 24;
        payload[offset+4] = value >> 32;
        payload[offset+5] = value >> 40;
        payload[offset+6] = value >> 48;
        payload[offset+7] = value >> 56;
}



