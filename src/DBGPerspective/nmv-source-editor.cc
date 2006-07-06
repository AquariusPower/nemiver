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

#include <gtkmm/table.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include "nmv-exception.h"
#include "nmv-source-editor.h"
#include "nmv-ustring.h"

using namespace nemiver::common ;

namespace nemiver {

struct SourceEditor::Priv {
    UString root_dir ;
    gint current_column ;
    gint current_line ;
    SourceView *source_view ;
    Gtk::HBox *status_box ;
    Gtk::Label *line_col_label ;
    Gtk::Label *line_count;
    sigc::signal<void, gint, gint> signal_insertion_moved ;

    //**************
    //<signal slots>
    //**************
    void on_mark_set_signal (const Gtk::TextBuffer::iterator &a_iter,
                             const Glib::RefPtr<Gtk::TextBuffer::Mark> &a_mark)
    {
        if (a_mark->get_name () == "insert") {
            update_line_col_info_from_iter (a_iter) ;
        }
    }

    void on_signal_insert (const Gtk::TextBuffer::iterator &a_iter,
                           const Glib::ustring &a_text,
                           int a_unknown)
    {
        update_line_col_info_from_iter (a_iter) ;
    }

    void on_signal_insertion_moved (gint a_line, gint a_col)
    {
        update_line_col_label ();
    }
    //**************
    //</signal slots>
    //**************

    void init_signals ()
    {
        source_view->get_buffer ()->signal_mark_set ().connect
            (sigc::mem_fun (*this, &SourceEditor::Priv::on_mark_set_signal)) ;
        source_view->get_buffer ()->signal_insert ().connect
            (sigc::mem_fun (*this, &SourceEditor::Priv::on_signal_insert)) ;
        signal_insertion_moved.connect
            (sigc::mem_fun (*this,
                            &SourceEditor::Priv::on_signal_insertion_moved)) ;
    }

    void update_line_col_info_from_iter (const Gtk::TextBuffer::iterator &a_iter)
    {
        current_line = a_iter.get_line () + 1;
        current_column = get_column_from_iter (a_iter) ;
        signal_insertion_moved.emit  (current_line, current_column) ;
    }

    void update_line_col_label ()
    {
        gint line_count = 0 ;
        if (source_view && source_view->get_buffer ()) {
            line_count = source_view->get_buffer ()->get_line_count () ;
        }
        line_col_label->set_text
            (UString ("line: ") + UString::from_int (current_line)
             + UString (", column: " + UString::from_int (current_column))
             + UString (", lines: ") + UString::from_int (line_count)) ;
    }

    gint get_column_from_iter (const Gtk::TextBuffer::iterator &a_iter)
    {
        //TODO: code this !
        return 0 ;
    }

    bool get_absolute_resource_path (const UString &a_relative_path,
                                     string &a_absolute_path)
    {
        bool result (false) ;
        string absolute_path =
            Glib::build_filename (Glib::locale_from_utf8 (root_dir),
                                  a_relative_path) ;
        if (Glib::file_test (absolute_path,
                             Glib::FILE_TEST_IS_REGULAR
                             | Glib::FILE_TEST_EXISTS)) {
            result = true;
            a_absolute_path = absolute_path ;
        } else {
            LOG ("could not find file: " << a_absolute_path) ;
        }
        return result ;
    }

    Priv () :
        current_column (1),
        current_line (1),
        source_view (Gtk::manage (new SourceView)),
        status_box (Gtk::manage (new Gtk::HBox )),
        line_col_label (Gtk::manage (new Gtk::Label ()))

    {
        update_line_col_label () ;
        status_box->pack_end (*line_col_label, Gtk::PACK_SHRINK) ;
        init_signals () ;
    }

    Priv (const UString &a_root_dir,
          Glib::RefPtr<SourceBuffer> &a_buf) :
        root_dir (a_root_dir),
        current_column (1),
        current_line (1),
        source_view (Gtk::manage (new SourceView (a_buf))),
        status_box (Gtk::manage (new Gtk::HBox)),
        line_col_label (Gtk::manage (new Gtk::Label ()))
    {
        update_line_col_label () ;
        status_box->pack_end (*line_col_label, Gtk::PACK_SHRINK) ;
        init_signals () ;
    }
};//end class SourceEditor

void
SourceEditor::init ()
{
    Gtk::ScrolledWindow *scrolled (Gtk::manage (new Gtk::ScrolledWindow));
    scrolled->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC) ;
    scrolled->add (*m_priv->source_view) ;
    scrolled->show_all () ;
    pack_start (*scrolled) ;
    pack_end (*m_priv->status_box, Gtk::PACK_SHRINK) ;

    string path ;
    if (!m_priv->get_absolute_resource_path ("icons/breakpoint-marker.png",
                                             path)) {
        THROW ("could not get path to breakpoint-marker.png") ;
    }

    Glib::RefPtr<Gdk::Pixbuf> bm_pixbuf = Gdk::Pixbuf::create_from_file (path) ;
    source_view ().set_marker_pixbuf ("breakpoint-marker", bm_pixbuf) ;

    source_view ().set_show_line_markers (true) ;
    source_view ().set_show_line_numbers (true);
}

SourceEditor::SourceEditor ()
{
    m_priv = new Priv ;
    init () ;
}

SourceEditor::SourceEditor (const UString &a_root_dir,
                            Glib::RefPtr<SourceBuffer> &a_buf)
{
    m_priv = new Priv (a_root_dir, a_buf) ;
    init () ;
}

SourceEditor::~SourceEditor ()
{
    LOG ("deleted") ;
}

SourceView&
SourceEditor::source_view ()
{
    return *m_priv->source_view ;
}

gint
SourceEditor::current_line ()
{
    return m_priv->current_line ;
}

void
SourceEditor::current_line (gint &a_line)
{
    m_priv->current_line = a_line;
}

gint
SourceEditor::current_column ()
{
    return m_priv->current_column ;
}

void
SourceEditor::current_column (gint &a_col)
{
    m_priv->current_column = a_col ;
}

}//end namespace nemiver

