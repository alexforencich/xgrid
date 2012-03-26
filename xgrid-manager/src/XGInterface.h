/************************************************************************/
/* XGInterface                                                          */
/*                                                                      */
/* XGrid Packet Interface                                               */
/*                                                                      */
/* XGInterface.h                                                        */
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

#ifndef __XG_INTERFACE_H
#define __XG_INTERFACE_H

#include <gtkmm.h>

#include "SerialInterface.h"

#include <string>
#include <tr1/memory>
#include <vector>
#include <deque>
#include <inttypes.h>
#include "XGPacket.h"

// XGInterface class
class XGInterface
{
public:
        XGInterface();
        virtual ~XGInterface();
        
        void set_serial_interface(std::tr1::shared_ptr<SerialInterface> si);
        void clear_serial_interface();
        bool has_serial_interface();
        
        void reset_buffer();
        
        void send_packet(XGPacket &pkt);
        
        void set_id(uint16_t id);
        
        bool set_debug(bool d);
        bool get_debug();
        
        sigc::signal<void, XGPacket> signal_receive_packet();
        sigc::signal<void, const char*, size_t> signal_send_raw_data();
        sigc::signal<void, const char*, size_t> signal_receive_raw_data();
        sigc::signal<void> signal_error();
        
protected:
        void on_receive_data();
        
        std::tr1::shared_ptr<SerialInterface> ser_int;
        
        std::deque<char> read_data_queue;
        
        uint16_t my_id;
        
        bool debug;
        
        uint8_t current_seq;
        
        sigc::signal<void, XGPacket> m_signal_receive_packet;
        sigc::signal<void, const char*, size_t> m_signal_send_raw_data;
        sigc::signal<void, const char*, size_t> m_signal_receive_raw_data;
        sigc::signal<void> m_signal_error;
        
        sigc::connection c_port_opened;
        sigc::connection c_port_closed;
        sigc::connection c_port_receive_data;
};

#endif //__XG_INTERFACE_H
