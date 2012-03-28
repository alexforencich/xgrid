/************************************************************************/
/* XGPacketBuilder                                                      */
/*                                                                      */
/* XGrid Packet Builder                                                 */
/*                                                                      */
/* XGPacketBuilder.h                                                    */
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

#ifndef __XG_PACKET_BUILDER_H
#define __XG_PACKET_BUILDER_H

#include <gtkmm.h>

#include "XGPacket.h"

// XGPacketBuilder class
class XGPacketBuilder : public Gtk::VBox
{
public:
        XGPacketBuilder();
        virtual ~XGPacketBuilder();
        
        void read_packet();
        
        void set_packet(XGPacket p);
        XGPacket get_packet();
        
        sigc::signal<void> signal_changed();
        
protected:
        //Signal handlers:
        void on_source_change();
        void on_type_change();
        void on_seq_change();
        void on_flags_change();
        void on_radius_change();
        void on_data_change();
        void on_hex_data_toggle();
        
        void update_packet();
        void update_data();
        
        unsigned long parse_number(Glib::ustring str);
        
        //Child widgets:
        Gtk::Table tbl;
        Gtk::Label lbl_source;
        Gtk::Entry ent_source;
        Gtk::Label lbl_type;
        Gtk::Entry ent_type;
        Gtk::Label lbl_seq;
        Gtk::Entry ent_seq;
        Gtk::Label lbl_flags;
        Gtk::Entry ent_flags;
        Gtk::Label lbl_radius;
        Gtk::Entry ent_radius;
        Gtk::Label lbl_data;
        Gtk::ScrolledWindow sw_data;
        Gtk::TextView tv_data;
        Gtk::Alignment al_hex_data;
        Gtk::CheckButton hex_data;
        
        bool updating_fields;
        
        XGPacket pkt;
        
        sigc::signal<void> m_signal_changed;
        
};

#endif //__XG_PACKET_BUILDER_H
