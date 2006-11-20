//Author: Dodji Seketeli
/*
 *This file is part of the Nemiver project
 *
 *Nemiver is free software; you can redistribute
 *it and/or modify it under the terms of
 *the GNU General Public License as published by the
 *Free Software Foundation; either version 2,
 *or (at your option) any later version.
 *
 *Nemiver is distributed in the hope that it will
 *be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU General Public License for more details.
 *
 *You should have received a copy of the
 *GNU General Public License along with Goupil;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */
#include "nmv-terminal.h"
#include <pty.h>
#include <iostream>
#include "nmv-ustring.h"
#include <gtkmm/bin.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/adjustment.h>
#include <vte/vte.h>
#include "nmv-exception.h"
#include "nmv-log-stream-utils.h"
#include <utmp.h>

NEMIVER_BEGIN_NAMESPACE(nemiver)

struct Terminal::Priv {
    //the master pty of the terminal (and of the whole process)
    int master_pty ;
    int slave_pty ;
    //the real vte terminal widget
    VteTerminal *vte ;
    //the same object as
    //m_vte, but wrapped as a Gtk::Widget
    Gtk::Widget *widget ;
    Gtk::Adjustment *adjustment ;

    Priv () :
        master_pty (0),
        slave_pty (0),
        vte (0),
        widget (0),
        adjustment (0)
    {
        GtkWidget *w = vte_terminal_new () ;
        vte = VTE_TERMINAL (w) ;
        THROW_IF_FAIL (vte) ;
        vte_terminal_set_scroll_on_output (vte, TRUE) ;
        vte_terminal_set_scrollback_lines (vte, 1000) ;
        vte_terminal_set_emulation (vte, "xterm") ;

        widget = Glib::wrap (w) ;
        THROW_IF_FAIL (widget) ;

        adjustment = Glib::wrap (vte_terminal_get_adjustment (vte)) ;
        THROW_IF_FAIL (adjustment) ;

        widget->reference () ;
        THROW_IF_FAIL (init_pty ()) ;
    }

    ~Priv ()
    {
        if (slave_pty) {
            close (slave_pty) ;
            slave_pty = 0 ;
        }

        if (master_pty) {
            close (master_pty) ;
            master_pty = 0 ;
        }

        if (widget) {
            widget->unreference () ;
            widget = 0;
            vte = 0;
        }
    }

    bool init_pty ()
    {
        if (openpty (&master_pty, &slave_pty, NULL, NULL, NULL)) {
            LOG_ERROR ("oops") ;
            return false ;
        }
        THROW_IF_FAIL (slave_pty) ;
        THROW_IF_FAIL (master_pty) ;

        if (grantpt (master_pty)) {
            LOG_ERROR ("oops") ;
            return false ;
        }

        if (unlockpt (master_pty)) {
            LOG_ERROR ("oops") ;
            return false ;
        }

        vte_terminal_set_pty (vte, master_pty) ;
        return true ;
    }
};//end Terminal::Priv

Terminal::Terminal ()
{
    m_priv.reset (new Priv) ;
}

Terminal::~Terminal ()
{
    LOG_D ("deleted, ", "destructor-domain") ;
}


Gtk::Widget&
Terminal::widget () const
{
    THROW_IF_FAIL (m_priv->widget && m_priv->vte) ;
    return *m_priv->widget ;
}

Gtk::Adjustment&
Terminal::adjustment () const
{
    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->adjustment) ;
    return *m_priv->adjustment ;
}

UString
Terminal::slave_pts_name () const
{
    THROW_IF_FAIL (m_priv) ;
    UString result ;

    if (!m_priv->master_pty) {
        LOG_ERROR ("oops") ;
        return result;
    }

    result = ptsname (m_priv->master_pty) ;
    return result ;
}

NEMIVER_END_NAMESPACE(nemiver)

