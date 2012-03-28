/************************************************************************/
/* XGPacketBuilder                                                      */
/*                                                                      */
/* XGrid Packet Builder                                                 */
/*                                                                      */
/* XGPacketBuilder.cpp                                                  */
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

#include "XGPacketBuilder.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

XGPacketBuilder::XGPacketBuilder()
{
        updating_fields = false;
        
        tbl.resize(7, 2);
        tbl.set_col_spacings(10);
        tbl.set_row_spacings(5);
        pack_start(tbl, false, false, 0);
        
        lbl_source.set_label("Source ID:");
        lbl_source.set_size_request(120, -1);
        tbl.attach(lbl_source, 0, 1, 0, 1);
        
        ent_source.set_text("0x0000");
        ent_source.signal_changed().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_source_change) );
        tbl.attach(ent_source, 1, 2, 0, 1);
        
        lbl_type.set_label("Type:");
        tbl.attach(lbl_type, 0, 1, 1, 2);
        
        ent_type.set_text("0x00");
        ent_type.signal_changed().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_type_change) );
        tbl.attach(ent_type, 1, 2, 1, 2);
        
        lbl_seq.set_label("Sequence:");
        tbl.attach(lbl_seq, 0, 1, 2, 3);
        
        ent_seq.set_text("0");
        ent_seq.signal_changed().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_seq_change) );
        tbl.attach(ent_seq, 1, 2, 2, 3);
        
        lbl_flags.set_label("Flags:");
        tbl.attach(lbl_flags, 0, 1, 3, 4);
        
        ent_flags.set_text("0x00");
        ent_flags.signal_changed().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_flags_change) );
        tbl.attach(ent_flags, 1, 2, 3, 4);
        
        lbl_radius.set_label("Radius:");
        tbl.attach(lbl_radius, 0, 1, 4, 5);
        
        ent_radius.set_text("0");
        ent_radius.signal_changed().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_radius_change) );
        tbl.attach(ent_radius, 1, 2, 4, 5);
        
        lbl_data.set_label("Data:");
        tbl.attach(lbl_data, 0, 1, 5, 6);
        
        tv_data.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
        tv_data.get_buffer()->signal_changed().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_data_change) );
        sw_data.add(tv_data);
        sw_data.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
        sw_data.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        tbl.attach(sw_data, 1, 2, 5, 7);
        
        hex_data.set_label("Hex");
        hex_data.set_active(true);
        hex_data.signal_toggled().connect( sigc::mem_fun(*this, &XGPacketBuilder::on_hex_data_toggle) );
        al_hex_data.add(hex_data);
        al_hex_data.set(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, 0, 0);
        tbl.attach(al_hex_data, 0, 1, 6, 7);
        
}


XGPacketBuilder::~XGPacketBuilder()
{
        
}


void XGPacketBuilder::on_source_change()
{
        if (updating_fields)
                return;
        
        pkt.source_id = parse_number(ent_source.get_text());
}


void XGPacketBuilder::on_type_change()
{
        if (updating_fields)
                return;
        
        pkt.type = parse_number(ent_type.get_text());
}


void XGPacketBuilder::on_seq_change()
{
        if (updating_fields)
                return;
        
        pkt.seq = parse_number(ent_seq.get_text());
}


void XGPacketBuilder::on_flags_change()
{
        if (updating_fields)
                return;
        
        pkt.flags = parse_number(ent_flags.get_text());
}


void XGPacketBuilder::on_radius_change()
{
        if (updating_fields)
                return;
        
        pkt.radius = parse_number(ent_radius.get_text());
}


void XGPacketBuilder::on_data_change()
{
        if (updating_fields)
                return;
        
        pkt.data.clear();
        
        Glib::ustring str = tv_data.get_buffer()->get_text();
        
        if (hex_data.get_active())
        {
                for (int i = 0; i < str.size(); i++)
                {
                        int k, num;
                        uint8_t b;
                        
                        if (sscanf(str.c_str()+i, "%2x%n", &k, &num) > 0)
                        {
                                i += num-1;
                                b = k;
                                pkt.data.push_back(b);
                        }
                }
        }
        else
        {
                for (int i = 0; i < str.size(); i++)
                {
                        pkt.data.push_back(str[i]);
                }
        }
        
        update_packet();
}


void XGPacketBuilder::on_hex_data_toggle()
{
        update_data();
}


void XGPacketBuilder::update_packet()
{
        pkt.build_packet();
        
        m_signal_changed.emit();
}


void XGPacketBuilder::update_data()
{
        std::stringstream ss;
        
        for (int j = 0; j < pkt.data.size(); j++)
        {
                if (hex_data.get_active())
                {
                        if (j > 0)
                                ss << " ";
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)pkt.data[j];
                }
                else
                {
                        ss << pkt.data[j];
                }
        }
        
        updating_fields = true;
        
        tv_data.get_buffer()->set_text(Glib::convert(ss.str(), "utf-8", "iso-8859-1"));
        
        updating_fields = false;
}


unsigned long XGPacketBuilder::parse_number(Glib::ustring str)
{
        unsigned long l = 0;
        size_t n = str.find("0x");
        
        if (n != std::string::npos)
        {
                l = std::strtol(str.substr(n).c_str(), 0, 16);
        }
        else
        {
                l = std::strtol(str.c_str(), 0, 10);
        }
        
        return l;
}


void XGPacketBuilder::read_packet()
{
        int row;
        std::stringstream ss;
        
        updating_fields = true;
        
        ent_source.set_text("0x" + Glib::ustring::format(std::setfill(L'0'), std::setw(4), std::hex, (int)pkt.source_id));
        ent_type.set_text("0x" + Glib::ustring::format(std::setfill(L'0'), std::setw(2), std::hex, (int)pkt.type));
        ent_seq.set_text(Glib::ustring::format(std::dec, (int)pkt.seq));
        ent_flags.set_text("0x" + Glib::ustring::format(std::setfill(L'0'), std::setw(2), std::hex, (int)pkt.flags));
        ent_radius.set_text(Glib::ustring::format(std::dec, (int)pkt.radius));
        
        updating_fields = false;
        
        update_data();
        
        m_signal_changed.emit();
}


void XGPacketBuilder::set_packet(XGPacket p)
{
        pkt = p;
        
        read_packet();
}


XGPacket XGPacketBuilder::get_packet()
{
        return pkt;
}


sigc::signal<void> XGPacketBuilder::signal_changed()
{
        return m_signal_changed;
}




