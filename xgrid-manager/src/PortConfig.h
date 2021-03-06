/************************************************************************/
/* PortConfig                                                           */
/*                                                                      */
/* ZigBee Terminal - Port Configuration                                 */
/*                                                                      */
/* PortConfig.h                                                         */
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

#ifndef __PORTCONFIG_H
#define __PORTCONFIG_H

#include "SerialInterface.h"

#include <string>

#include <gtkmm.h>

// PortConfig class
class PortConfig : public Gtk::Dialog
{
public:
        PortConfig();
        virtual ~PortConfig();
        
        void refresh_ports();
        
        void set_port(Glib::ustring p);
        void set_baud(unsigned long b);
        void set_parity(SerialInterface::SerialParity p);
        void set_bits(int b);
        void set_stop_bits(int sb);
        void set_flow_control(SerialInterface::SerialFlow f);
        
        Glib::ustring get_port();
        unsigned long get_baud();
        SerialInterface::SerialParity get_parity();
        int get_bits();
        int get_stop_bits();
        SerialInterface::SerialFlow get_flow_control();
        
protected:
        //Signal handlers:
        virtual void on_show();
        void on_ok_click();
        void on_cancel_click();
        
        Glib::ustring select_port(Glib::ustring p);
        unsigned long select_baud(unsigned long b);
        SerialInterface::SerialParity select_parity(SerialInterface::SerialParity p);
        int select_bits(int b);
        int select_stop_bits(int sb);
        SerialInterface::SerialFlow select_flow_control(SerialInterface::SerialFlow f);
        
        
        //Child widgets:
        Gtk::Button *btnOK;
        Gtk::Button *btnCancel;
        Gtk::Frame frame;
        Gtk::Table table;
        Gtk::Label label1;
        Gtk::Label label2;
        Gtk::Label label3;
        Gtk::Label label4;
        Gtk::Label label5;
        Gtk::Label label6;
        Gtk::ComboBoxText cmbtPort;
        Gtk::ComboBoxText cmbtSpeed;
        Gtk::ComboBoxText cmbtParity;
        Gtk::ComboBoxText cmbtBits;
        Gtk::ComboBoxText cmbtStopBits;
        Gtk::ComboBoxText cmbtFlowControl;
        
        Glib::ustring port;
        unsigned long baud;
        SerialInterface::SerialParity parity;
        int bits;
        int stop_bits;
        SerialInterface::SerialFlow flow_control;
};

// Prototypes


#endif //__PORTCONFIG_H
