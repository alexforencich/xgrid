/************************************************************************/
/* XGridManager                                                         */
/*                                                                      */
/* XGrid Manager                                                        */
/*                                                                      */
/* XGridManager.cpp                                                     */
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

#include "XGridManager.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

XGridManager::XGridManager()
{
        set_title("XGrid Manager");
        set_position(Gtk::WIN_POS_CENTER);
        
        add(vbox1);
        
        // Main menu
        
        vbox1.pack_start(main_menu, false, true, 0);
        
        file_menu_item.set_label("_File");
        file_menu_item.set_use_underline(true);
        main_menu.append(file_menu_item);
        
        file_menu_item.set_submenu(file_menu);
        
        file_quit_item.set_label(Gtk::Stock::QUIT.id);
        file_quit_item.set_use_stock(true);
        file_quit_item.signal_activate().connect( sigc::mem_fun(*this, &XGridManager::on_file_quit_item_activate) );
        file_menu.append(file_quit_item);
        
        view_menu_item.set_label("_View");
        view_menu_item.set_use_underline(true);
        main_menu.append(view_menu_item);
        
        view_menu_item.set_submenu(view_menu);
        
        view_clear_item.set_label("Clear");
        view_clear_item.signal_activate().connect( sigc::mem_fun(*this, &XGridManager::on_view_clear_activate) );
        view_menu.append(view_clear_item);
        
        config_menu_item.set_label("_Config");
        config_menu_item.set_use_underline(true);
        main_menu.append(config_menu_item);
        
        config_menu_item.set_submenu(config_menu);
        
        config_port_item.set_label("Port");
        config_port_item.signal_activate().connect( sigc::mem_fun(*this, &XGridManager::on_config_port_item_activate) );
        config_menu.append(config_port_item);
        
        config_close_port_item.set_label("Close Port");
        config_close_port_item.signal_activate().connect( sigc::mem_fun(*this, &XGridManager::on_config_close_port_item_activate) );
        config_menu.append(config_close_port_item);
        
        // Tabs
        note.set_size_request(500, 400);
        vbox1.pack_start(note, true, true, 0);
        
        // Packet Log Tab
        note.append_page(vpane_pkt_log, "Packet Log");
        
        tv_pkt_log_tm = Gtk::ListStore::create(cPacketLogModel);
        tv_pkt_log.set_model(tv_pkt_log_tm);
        tv_pkt_log.signal_cursor_changed().connect( sigc::mem_fun(*this, &XGridManager::on_tv_pkt_log_cursor_changed) );
        
        tv_pkt_log.append_column("Src", cPacketLogModel.source_id);
        tv_pkt_log.append_column("T", cPacketLogModel.type);
        tv_pkt_log.append_column("S", cPacketLogModel.seq);
        tv_pkt_log.append_column("F", cPacketLogModel.flags);
        tv_pkt_log.append_column("R", cPacketLogModel.radius);
        tv_pkt_log.append_column("Data", cPacketLogModel.data);
        
        tv_pkt_log.modify_font(Pango::FontDescription("monospace"));
        
        sw_pkt_log.add(tv_pkt_log);
        sw_pkt_log.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        vpane_pkt_log.pack1(sw_pkt_log, true, true);
        
        tv2_pkt_log.set_size_request(400,100);
        tv2_pkt_log.modify_font(Pango::FontDescription("monospace"));
        tv2_pkt_log.set_editable(false);
        tv2_pkt_log.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
        
        sw2_pkt_log.add(tv2_pkt_log);
        sw2_pkt_log.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        vpane_pkt_log.pack2(sw2_pkt_log, false, false);
        
        // status bar
        
        status.push("Not connected");
        vbox1.pack_start(status, false, true, 0);
        
        baud = 115200;
        port = "";
        parity = SerialInterface::SP_None;
        bits = 8;
        stop_bits = 1;
        flow_control = SerialInterface::SF_None;
        
        dlgPort.set_port(port);
        dlgPort.set_baud(baud);
        dlgPort.set_parity(parity);
        dlgPort.set_bits(bits);
        dlgPort.set_stop_bits(stop_bits);
        dlgPort.set_flow_control(flow_control);
        
        ser_int = std::tr1::shared_ptr<SerialInterface>(new SerialInterface());
        
        ser_int->port_opened().connect( sigc::mem_fun(*this, &XGridManager::on_port_open) );
        ser_int->port_closed().connect( sigc::mem_fun(*this, &XGridManager::on_port_close) );
        //ser_int->port_error().connect( sigc::mem_fun(*this, &XGridManager::on_port_error) );
        //ser_int->port_receive_data().connect( sigc::mem_fun(*this, &XGridManager::on_port_receive_data) );
        
        ser_int->set_debug(true);
        
        xg_int.set_serial_interface(ser_int);
        
        xg_int.signal_receive_packet().connect( sigc::mem_fun(*this, &XGridManager::on_receive_packet) );
        
        show_all_children();
}


XGridManager::~XGridManager()
{
        
}


void XGridManager::on_file_quit_item_activate()
{
        gtk_main_quit();
}


void XGridManager::on_config_port_item_activate()
{
        int response;
        
        dlgPort.set_port(port);
        dlgPort.set_baud(baud);
        dlgPort.set_parity(parity);
        dlgPort.set_bits(bits);
        dlgPort.set_stop_bits(stop_bits);
        dlgPort.set_flow_control(flow_control);
        response = dlgPort.run();
        
        if (response == Gtk::RESPONSE_OK)
        {
                close_port();
                
                port = dlgPort.get_port();
                baud = dlgPort.get_baud();
                parity = dlgPort.get_parity();
                bits = dlgPort.get_bits();
                stop_bits = dlgPort.get_stop_bits();
                flow_control = dlgPort.get_flow_control();
                
                open_port();
        }
}


void XGridManager::on_config_close_port_item_activate()
{
        close_port();
}


void XGridManager::on_view_clear_activate()
{
        read_data_queue.clear();
        tv_pkt_log_tm->clear();
}


void XGridManager::on_tv_pkt_log_cursor_changed()
{
        Gtk::TreeModel::iterator it = tv_pkt_log.get_selection()->get_selected();
        Gtk::TreeModel::Row row = *it;
        //ZigBeePacket pkt = (ZigBeePacket)row[cPacketLogModel.Packet];
        
        //tv2_pkt_log.get_buffer()->set_text(pkt.get_desc());
}


void XGridManager::on_port_open()
{
        gsize num;
        
        std::cout << "on_port_open()" << std::endl;
        
        port = ser_int->get_port();
        baud = ser_int->get_baud();
        
        status.pop();
        status.push(ser_int->get_status_string());
}


void XGridManager::on_port_close()
{
        status.pop();
        status.push(ser_int->get_status_string());
}


void XGridManager::on_receive_packet(XGPacket pkt)
{
        Gtk::TreeModel::iterator it = tv_pkt_log_tm->append();
        Gtk::TreePath path = Gtk::TreePath(it);
        Gtk::TreeModel::Row row = *it;
        row[cPacketLogModel.packet] = pkt;
        row[cPacketLogModel.source_id] = Glib::ustring::format(std::hex, std::setfill(L'0'), std::setw(4), pkt.source_id);
        row[cPacketLogModel.type] = pkt.type;
        row[cPacketLogModel.seq] = pkt.seq;
        row[cPacketLogModel.flags] = pkt.flags;
        row[cPacketLogModel.radius] = pkt.radius;
        row[cPacketLogModel.data] = pkt.get_hex_packet();
        tv_pkt_log.scroll_to_row(path);
}


void XGridManager::open_port()
{
        if (ser_int->is_open())
                close_port();
        
        read_data_queue.clear();
        
        ser_int->set_port(port);
        ser_int->set_baud(baud);
        ser_int->set_parity(parity);
        ser_int->set_bits(bits);
        ser_int->set_stop(stop_bits);
        ser_int->set_flow(flow_control);
        ser_int->open_port();
}


void XGridManager::close_port()
{
        ser_int->close_port();
}



