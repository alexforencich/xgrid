/************************************************************************/
/* XGridManager                                                         */
/*                                                                      */
/* XGrid Manager                                                        */
/*                                                                      */
/* XGridManager.h                                                       */
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

#ifndef __XGridManager_H
#define __XGridManager_H

#include <gtkmm.h>

#include <tr1/memory>

#include "PortConfig.h"
#include "SerialInterface.h"
#include "XGPacket.h"
#include "XGInterface.h"
#include "XGPacketBuilder.h"

// XGridManager class
class XGridManager : public Gtk::Window
{
public:
        XGridManager();
        virtual ~XGridManager();
        
protected:
        //Signal handlers:
        void on_file_quit_item_activate();
        void on_config_port_item_activate();
        void on_config_close_port_item_activate();
        
        void on_view_clear_activate();
        
        void on_tv_pkt_log_cursor_changed();
        
        void on_pkt_builder_change();
        void on_btn_pkt_builder_send_click();
        
        void on_btn_node_query_click();
        void on_btn_node_reset_click();
        
        void send_packet(XGPacket &pkt);
        
        void on_port_open();
        void on_port_close();
        
        void on_receive_packet(XGPacket pkt);
        
        void open_port();
        void close_port();
        
        // Tree model columns
        class PacketLogModel : public Gtk::TreeModel::ColumnRecord
        {
        public:
                PacketLogModel()
                {
                        add(packet);
                        add(source_id);
                        add(type);
                        add(seq);
                        add(flags);
                        add(radius);
                        add(data);
                }
                
                Gtk::TreeModelColumn<XGPacket> packet;
                Gtk::TreeModelColumn<Glib::ustring> source_id;
                Gtk::TreeModelColumn<uint8_t> type;
                Gtk::TreeModelColumn<uint8_t> seq;
                Gtk::TreeModelColumn<uint8_t> flags;
                Gtk::TreeModelColumn<uint8_t> radius;
                Gtk::TreeModelColumn<Glib::ustring> data;
        };
        
        PacketLogModel cPacketLogModel;
        
        Glib::RefPtr<Gtk::ListStore> tv_pkt_log_tm;
        
        //Child widgets:
        // window
        Gtk::VBox vbox1;
        // menu bar
        Gtk::MenuBar main_menu;
        Gtk::MenuItem file_menu_item;
        Gtk::Menu file_menu;
        Gtk::ImageMenuItem file_quit_item;
        Gtk::MenuItem view_menu_item;
        Gtk::Menu view_menu;
        Gtk::ImageMenuItem view_clear_item;
        Gtk::MenuItem config_menu_item;
        Gtk::Menu config_menu;
        Gtk::ImageMenuItem config_port_item;
        Gtk::ImageMenuItem config_close_port_item;
        // tabs
        Gtk::Notebook note;
        // packet log
        Gtk::VPaned vpane_pkt_log;
        Gtk::ScrolledWindow sw_pkt_log;
        Gtk::TreeView tv_pkt_log;
        Gtk::ScrolledWindow sw2_pkt_log;
        Gtk::TextView tv2_pkt_log;
        // node info
        Gtk::VBox vbox_node_info;
        Gtk::Frame frame_node_firmware_info;
        Gtk::VBox vbox_node_firmware_info;
        Gtk::Label lbl_node_build;
        Gtk::Label lbl_node_crc;
        Gtk::HButtonBox bbox_node_info;
        Gtk::Button btn_node_query;
        Gtk::Button btn_node_reset;
        // packet builder
        Gtk::VBox vbox_pkt_builder;
        Gtk::VPaned vpane_pkt_builder;
        Gtk::HButtonBox bbox_pkt_builder;
        Gtk::Button btn_pkt_builder_send;
        Gtk::ScrolledWindow sw_pkt_builder;
        XGPacketBuilder pkt_builder;
        Gtk::ScrolledWindow sw2_pkt_builder;
        Gtk::TextView tv_pkt_builder;
        
        // status bar
        Gtk::Statusbar status;
        
        PortConfig dlgPort;
        
        Glib::ustring port;
        unsigned long baud;
        SerialInterface::SerialParity parity;
        int bits;
        int stop_bits;
        SerialInterface::SerialFlow flow_control;
        
        std::tr1::shared_ptr<SerialInterface> ser_int;
        
        XGInterface xg_int;
        
        std::deque<char> read_data_queue;
        
        
};

#endif //__XGridManager_H
