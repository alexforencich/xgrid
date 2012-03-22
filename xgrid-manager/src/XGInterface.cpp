/************************************************************************/
/* XGInterface                                                          */
/*                                                                      */
/* XGrid Packet Interface                                               */
/*                                                                      */
/* XGInterface.cpp                                                      */
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

#include "XGInterface.h"

#include <iostream>
#include <sstream>
#include <iomanip>

XGInterface::XGInterface() :
        current_seq(0),
        my_id(0),
        debug(false)
{
        // nothing
}


XGInterface::~XGInterface()
{
        // nothing
}


void XGInterface::set_serial_interface(std::tr1::shared_ptr<SerialInterface> si)
{
        clear_serial_interface();
        if (!si)
                return;
        ser_int = si;
        c_port_opened = ser_int->port_opened().connect( sigc::mem_fun(*this, &XGInterface::reset_buffer) );
        c_port_closed = ser_int->port_closed().connect( sigc::mem_fun(*this, &XGInterface::reset_buffer) );
        c_port_receive_data = ser_int->port_receive_data().connect( sigc::mem_fun(*this, &XGInterface::on_receive_data) );
}


void XGInterface::clear_serial_interface()
{
        if (!ser_int)
                return;
        c_port_opened.disconnect();
        c_port_closed.disconnect();
        c_port_receive_data.disconnect();
        ser_int = std::tr1::shared_ptr<SerialInterface>();
}


bool XGInterface::has_serial_interface()
{
        return ser_int;
}


void XGInterface::reset_buffer()
{
        read_data_queue.clear();
}


void XGInterface::send_packet(XGPacket pkt)
{
        gsize num;
        int ret;
        int len;
        char *ptr;
        
        pkt.seq = current_seq++;
        pkt.source_id = my_id;
        pkt.build_packet();
        
        if (!ser_int)
        {
                std::cerr << "[XGInterface] No SerialInterface associated!" << std::endl;
                m_signal_error.emit();
                return;
        }
        
        std::vector<uint8_t> data = pkt.get_raw_packet();
        
        len = data.size();
        ptr = (char *)&data[0];
        
        // write packet
        while (len > 0)
        {
                ret = ser_int->write(ptr, len, num);
                
                if (ret != SerialInterface::SS_Success)
                {
                        std::cerr << "[XGInterface] Error: unable to write packet!";
                        m_signal_error.emit();
                        return;
                }
                
                if (num > 0)
                        m_signal_send_raw_data.emit(ptr, num);
                
                len -= num;
                ptr += num;
        }
}


void XGInterface::set_id(uint16_t id)
{
        my_id = id;
}


bool XGInterface::set_debug(bool d)
{
        debug = d;
        return debug;
}


bool XGInterface::get_debug()
{
        return debug;
}


sigc::signal<void, XGPacket> XGInterface::signal_receive_packet()
{
        return m_signal_receive_packet;
}


sigc::signal<void, const char*, size_t> XGInterface::signal_send_raw_data()
{
        return m_signal_send_raw_data;
}


sigc::signal<void, const char*, size_t> XGInterface::signal_receive_raw_data()
{
        return m_signal_receive_raw_data;
}


void XGInterface::on_receive_data()
{
        gsize num;
        int status;
        static char buf[1024];
        XGPacket pkt;
        size_t len;
        
        if (!ser_int)
        {
                std::cerr << "[XGInterface] No SerialInterface associated!" << std::endl;
                m_signal_error.emit();
                return;
        }
        
        // read raw data from serial port
        do
        {
                status = ser_int->read(buf, 1024, num);
                
                if (status == SerialInterface::SS_Error)
                {
                        std::cerr << "[XGInterface] Read error!" << std::endl;
                        m_signal_error.emit();
                        ser_int->close_port();
                        return;
                }
                
                if (status == SerialInterface::SS_EOF)
                {
                        std::cerr << "[XGInterface] End of file!" << std::endl;
                        m_signal_error.emit();
                        ser_int->close_port();
                        return;
                }
                
                if (debug)
                {
                        std::cout << "[XGInterface] Read " << std::dec << num << " bytes" << std::endl;
                }
                
                for (int i = 0; i < num; i++)
                {
                        read_data_queue.push_back(buf[i]);
                }
                
                if (num > 0)
                        m_signal_receive_raw_data.emit(buf, num);
        }
        while (num == 1024);
        
        // try to read packets
        do
        {
                len = 0;
                if (pkt.read_packet(read_data_queue, len))
                {
                        pkt.decode_packet();
                        m_signal_receive_packet.emit(pkt);
                }
                for (int i = 0; i < len; i++)
                        read_data_queue.pop_front();
        }
        while (read_data_queue.size() > 0 && len > 0);
}




