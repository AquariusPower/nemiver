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
 *GNU General Public License along with Nemiver;
 *see the file COPYING.
 *If not, write to the Free Software Foundation,
 *Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *See COPYRIGHT file copyright information.
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <gtksourceviewmm/init.h>
#include <gtksourceviewmm/sourcelanguagesmanager.h>
#include "nmv-dbg-perspective.h"
#include "nmv-ui-utils.h"
#include "nmv-env.h"
#include "nmv-run-program-dialog.h"
#include "nmv-load-core-dialog.h"
#include "nmv-proc-list-dialog.h"
#include "nmv-ui-utils.h"
#include "nmv-sess-mgr.h"
#include "nmv-date-utils.h"
#include "nmv-call-stack.h"
#include "nmv-throbber.h"
#include "nmv-vars-editor.h"
#include "nmv-terminal.h"

using namespace std ;
using namespace nemiver::common ;
using namespace nemiver::ui_utils;
using namespace gtksourceview ;

namespace nemiver {

const char *SET_BREAKPOINT    = "nmv-set-breakpoint" ;
const char *CONTINUE          = "nmv-continue" ;
const char *STOP_DEBUGGER     = "nmv-stop-debugger" ;
const char *RUN_DEBUGGER      = "nmv-run-debugger" ;
const char *LINE_POINTER      = "nmv-line-pointer" ;
const char *RUN_TO_CURSOR     = "nmv-run-to-cursor" ;
const char *STEP_INTO         = "nmv-step-into" ;
const char *STEP_OVER         = "nmv-step-over" ;
const char *STEP_OUT          = "nmv-step-out" ;

const char *SESSION_NAME = "sessionname" ;
const char *PROGRAM_NAME= "programname" ;
const char *PROGRAM_ARGS= "programarguments" ;
const char *PROGRAM_CWD= "programcwd" ;

const Gtk::StockID STOCK_SET_BREAKPOINT (SET_BREAKPOINT) ;
const Gtk::StockID STOCK_CONTINUE (CONTINUE) ;
const Gtk::StockID STOCK_STOP_DEBUGGER (STOP_DEBUGGER) ;
const Gtk::StockID STOCK_RUN_DEBUGGER (RUN_DEBUGGER) ;
const Gtk::StockID STOCK_LINE_POINTER (LINE_POINTER) ;
const Gtk::StockID STOCK_RUN_TO_CURSOR (RUN_TO_CURSOR) ;
const Gtk::StockID STOCK_STEP_INTO (STEP_INTO) ;
const Gtk::StockID STOCK_STEP_OVER (STEP_OVER) ;
const Gtk::StockID STOCK_STEP_OUT (STEP_OUT) ;

class DBGPerspective : public IDBGPerspective {
    //non copyable
    DBGPerspective (const IPerspective&) ;
    DBGPerspective& operator= (const IPerspective&) ;
    struct Priv ;
    SafePtr<Priv> m_priv ;

private:

    struct SlotedButton : Gtk::Button {
        UString file_path;
        DBGPerspective *perspective ;

        SlotedButton () :
            Gtk::Button (),
            perspective (NULL)
        {}

        SlotedButton (const Gtk::StockID &a_id) :
            Gtk::Button (a_id),
            perspective (NULL)
        {}

        void on_clicked ()
        {
            if (perspective) {
                perspective->close_file (file_path) ;
            }
        }

        ~SlotedButton ()
        {
        }
    };

    //************
    //<signal slots>
    //************
    void on_open_action () ;
    void on_close_action () ;
    void on_execute_program_action () ;
    void on_load_core_file_action () ;
    void on_attach_to_program_action () ;
    void on_stop_debugger_action ();
    void on_run_action () ;
    void on_next_action () ;
    void on_step_into_action () ;
    void on_step_out_action () ;
    void on_continue_action () ;
    void on_toggle_breakpoint_action () ;
    void on_show_commands_action () ;
    void on_show_errors_action () ;
    void on_show_target_output_action () ;

    void on_switch_page_signal (GtkNotebookPage *a_page, guint a_page_num) ;

    void on_attached_to_target_signal (bool a_is_attached) ;

    void on_debugger_ready_signal (bool a_is_ready) ;

    void on_insert_in_command_view_signal (const Gtk::TextBuffer::iterator &a_iter,
                                           const Glib::ustring &a_text,
                                           int a_dont_know) ;

    void on_source_view_markers_region_clicked_signal (int a_line) ;

    bool on_button_pressed_in_source_view_signal (GdkEventButton *a_event) ;

    bool on_key_pressed_in_source_view_signal (GdkEventKey *a_event) ;

    void on_shutdown_signal () ;

    void on_show_command_view_changed_signal (bool) ;

    void on_show_target_output_view_changed_signal (bool) ;

    void on_show_log_view_changed_signal (bool) ;

    void on_debugger_console_message_signal (const UString &a_msg) ;

    void on_debugger_target_output_message_signal (const UString &a_msg);

    void on_debugger_log_message_signal (const UString &a_msg) ;

    void on_debugger_command_done_signal (const UString &a_command) ;

    void on_debugger_breakpoints_set_signal
                                (const map<int, IDebugger::BreakPoint> &) ;

    void on_debugger_breakpoint_deleted_signal
                                        (const IDebugger::BreakPoint&, int) ;

    void on_debugger_stopped_signal (const UString &a_reason,
                                     bool a_has_frame,
                                     const IDebugger::Frame &) ;
    void on_program_finished_signal () ;
    void on_frame_selected_signal (int, const IDebugger::Frame &) ;

    void on_debugger_running_signal () ;

    void on_signal_received_by_target_signal (const UString &a_signal,
                                              const UString &a_meaning) ;

    void on_debugger_error_signal (const UString &a_msg) ;

    void on_debugger_state_changed_signal (IDebugger::State a_state) ;
    //************
    //</signal slots>
    //************

    string build_resource_path (const UString &a_dir, const UString &a_name) ;
    void add_stock_icon (const UString &a_stock_id,
                         const UString &icon_dir,
                         const UString &icon_name) ;
    void add_perspective_menu_entries () ;
    void init_perspective_menu_entries () ;
    void add_perspective_toolbar_entries () ;
    void init_icon_factory () ;
    void init_actions () ;
    void init_toolbar () ;
    void init_body () ;
    void init_signals () ;
    void init_debugger_signals () ;
    void append_source_editor (SourceEditor &a_sv,
                               const UString &a_path) ;
    SourceEditor* get_current_source_editor () ;
    ISessMgr* get_session_manager () ;
    UString get_current_file_path () ;
    SourceEditor* get_source_editor_from_path (const UString &a_path) ;
    void bring_source_as_current (const UString &a_path) ;
    int get_n_pages () ;
    void popup_source_view_contextual_menu (GdkEventButton *a_event) ;
    void save_session () ;
    void save_session (ISessMgr::Session &a_session) ;
    IProcMgr* get_process_manager () ;

public:

    DBGPerspective () ;

    virtual ~DBGPerspective () ;

    void get_info (Info &a_info) const ;

    void do_init () ;

    void do_init (IWorkbenchSafePtr &a_workbench) ;

    const UString& get_perspective_identifier () ;

    void get_toolbars (list<Gtk::Widget*> &a_tbs)  ;

    Gtk::Widget* get_body ()  ;

    void edit_workbench_menu () ;

    void open_file () ;

    bool open_file (const UString &a_path,
                    int current_line=-1) ;

    void close_current_file () ;

    void close_file (const UString &a_path) ;

    ISessMgr& session_manager () ;

    void execute_session (ISessMgr::Session &a_session) ;

    void execute_program () ;

    void execute_program (const UString &a_prog_and_args,
                          const map<UString, UString> &a_env,
                          const UString &a_cwd=".") ;

    void execute_program (const UString &a_prog,
                          const UString &a_args,
                          const map<UString, UString> &a_env,
                          const UString &a_cwd=".") ;

    void attach_to_program () ;
    void attach_to_program (unsigned int a_pid) ;
    void load_core_file () ;
    void load_core_file (const UString &a_prog_file,
                         const UString &a_core_file_path) ;
    void run () ;
    void stop () ;
    void step_over () ;
    void step_into () ;
    void step_out () ;
    void do_continue () ;
    void set_breakpoint () ;
    void set_breakpoint (const UString &a_file,
                         int a_line) ;
    void append_breakpoints (const map<int, IDebugger::BreakPoint> &a_breaks) ;
    bool get_breakpoint_number (const UString &a_file_name,
                                int a_linenum,
                                int &a_break_num) ;
    bool delete_breakpoint () ;
    bool delete_breakpoint (int a_breakpoint_num) ;
    bool delete_breakpoint (const UString &a_file_path,
                            int a_linenum) ;
    bool is_breakpoint_set_at_line (const UString &a_file_path,
                                    int a_linenum) ;
    void toggle_breakpoint (const UString &a_file_path,
                            int a_linenum) ;
    void toggle_breakpoint () ;
    void append_visual_breakpoint (const UString &a_file_name,
                                   int a_linenum) ;
    void delete_visual_breakpoint (const UString &a_file_name, int a_linenum) ;
    void delete_visual_breakpoint (int a_breaknum) ;

    IDebuggerSafePtr& debugger () ;

    Gtk::TextView& get_command_view () ;

    Gtk::ScrolledWindow& get_command_view_scrolled_win () ;

    Gtk::TextView& get_target_output_view () ;

    Gtk::ScrolledWindow& get_target_output_view_scrolled_win () ;

    Gtk::TextView& get_log_view () ;

    Gtk::ScrolledWindow& get_log_view_scrolled_win () ;

    CallStack& get_call_stack () ;

    Gtk::ScrolledWindow& get_call_stack_scrolled_win () ;

    VarsEditor& get_variables_editor () ;

    Gtk::ScrolledWindow& get_variables_editor_scrolled_win () ;

    Terminal& get_terminal () ;

    Gtk::ScrolledWindow& get_terminal_scrolled_win () ;

    void set_show_command_view (bool) ;

    void set_show_target_output_view (bool) ;

    void set_show_log_view (bool) ;

    void set_show_call_stack_view (bool) ;

    void set_show_variables_editor_view (bool) ;

    void set_show_terminal_view (bool) ;

    void add_text_to_command_view (const UString &a_text,
                                   bool a_no_repeat=false) ;

    void add_text_to_target_output_view (const UString &a_text) ;

    void add_text_to_log_view (const UString &a_text) ;

    void set_where (const UString &a_path, int line) ;

    void unset_where () ;

    Gtk::Widget* get_contextual_menu () ;

    sigc::signal<void, bool>& show_command_view_signal () ;
    sigc::signal<void, bool>& show_target_output_view_signal () ;
    sigc::signal<void, bool>& show_log_view_signal () ;
    sigc::signal<void, bool>& activated_signal () ;
    sigc::signal<void, bool>& attached_to_target_signal () ;
    sigc::signal<void, bool>& debugger_ready_signal () ;
};//end class DBGPerspective

struct RefGObject {
    void operator () (Glib::Object *a_object)
    {
        if (a_object) {a_object->reference ();}
    }
};

struct UnrefGObject {
    void operator () (Glib::Object *a_object)
    {
        if (a_object) {a_object->unreference ();}
    }
};

struct RefGObjectNative {
    void operator () (void *a_object)
    {
        if (a_object && G_IS_OBJECT (a_object)) {
            g_object_ref (G_OBJECT (a_object));
        }
    }
};

struct UnrefGObjectNative {
    void operator () (void *a_object)
    {
        if (a_object && G_IS_OBJECT (a_object)) {
            g_object_unref (G_OBJECT (a_object)) ;
        }
    }
};


struct DBGPerspective::Priv {
    bool initialized ;
    bool reused_session ;
    UString prog_name ;
    UString prog_args ;
    UString prog_cwd ;
    Glib::RefPtr<Gtk::ActionGroup> target_connected_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> debugger_ready_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> debugger_busy_action_group ;
    Glib::RefPtr<Gtk::ActionGroup> default_action_group;
    Glib::RefPtr<Gtk::ActionGroup> opened_file_action_group;
    Glib::RefPtr<Gtk::UIManager> ui_manager ;
    Glib::RefPtr<Gtk::IconFactory> icon_factory ;
    Gtk::UIManager::ui_merge_id menubar_merge_id ;
    Gtk::UIManager::ui_merge_id toolbar_merge_id ;
    Gtk::UIManager::ui_merge_id contextual_menu_merge_id;
    Gtk::Widget *contextual_menu ;
    IWorkbenchSafePtr workbench ;
    SafePtr<Gtk::HBox> toolbar ;
    ThrobberSafePtr throbber ;
    sigc::signal<void, bool> activated_signal;
    sigc::signal<void, bool> attached_to_target_signal;
    sigc::signal<void, bool> debugger_ready_signal;
    sigc::signal<void, bool> show_command_view_signal  ;
    sigc::signal<void, bool> show_target_output_view_signal  ;
    sigc::signal<void, bool> show_log_view_signal ;
    bool command_view_is_visible ;
    bool target_output_view_is_visible ;
    bool log_view_is_visible ;
    bool call_stack_view_is_visible ;
    bool variables_editor_view_is_visible ;
    bool terminal_view_is_visible ;
    Glib::RefPtr<Gnome::Glade::Xml> body_glade ;
    SafePtr<Gtk::Window> body_window ;
    Glib::RefPtr<Gtk::Paned> body_main_paned ;
    Gtk::Notebook *sourceviews_notebook ;
    map<UString, int> path_2_pagenum_map ;
    map<int, SourceEditor*> pagenum_2_source_editor_map ;
    map<int, UString> pagenum_2_path_map ;
    Gtk::Notebook *statuses_notebook ;
    SafePtr<Gtk::TextView> command_view ;
    SafePtr<Gtk::ScrolledWindow> command_view_scrolled_win ;
    SafePtr<Gtk::TextView> target_output_view;
    SafePtr<Gtk::ScrolledWindow> target_output_view_scrolled_win;
    SafePtr<Gtk::TextView> log_view ;
    SafePtr<Gtk::ScrolledWindow> log_view_scrolled_win ;
    SafePtr<CallStack> call_stack ;
    SafePtr<Gtk::ScrolledWindow> call_stack_scrolled_win ;
    SafePtr<VarsEditor> variables_editor ;
    SafePtr<Gtk::ScrolledWindow> variables_editor_scrolled_win ;
    SafePtr<Terminal> terminal ;
    SafePtr<Gtk::ScrolledWindow> terminal_scrolled_win ;

    int current_page_num ;
    IDebuggerSafePtr debugger ;
    map<int, IDebugger::BreakPoint> breakpoints ;
    ISessMgrSafePtr session_manager ;
    ISessMgr::Session session ;
    IProcMgrSafePtr process_manager ;
    UString last_command_text ;

    Priv () :
        initialized (false),
        reused_session (false),
        menubar_merge_id (0),
        toolbar_merge_id (0),
        contextual_menu_merge_id(0),
        contextual_menu (NULL),
        workbench (NULL),
        command_view_is_visible (false),
        target_output_view_is_visible (false),
        log_view_is_visible (false),
        call_stack_view_is_visible (false),
        variables_editor_view_is_visible (false),
        terminal_view_is_visible (false),
        sourceviews_notebook (NULL),
        statuses_notebook (NULL),
        current_page_num (0)
    {}
};//end struct DBGPerspective::Priv

enum ViewsIndex{
    COMMAND_VIEW_INDEX=0,
    TARGET_OUTPUT_VIEW_INDEX,
    ERROR_VIEW_INDEX,
    CALL_STACK_VIEW_INDEX,
    VARIABLES_VIEW_INDEX,
    TERMINAL_VIEW_INDEX
};

#ifndef CHECK_P_INIT
#define CHECK_P_INIT THROW_IF_FAIL(m_priv && m_priv->initialized) ;
#endif

ostream&
operator<< (ostream &a_out,
            const IDebugger::Frame &a_frame)
{
    a_out << "file-full-name: " << a_frame.file_full_name () << "\n"
          << "file-name: "      << a_frame.file_name () << "\n"
          << "line number: "    << a_frame.line () << "\n";

    return a_out ;
}

//****************************
//<slots>
//***************************
void
DBGPerspective::on_open_action ()
{
    NEMIVER_TRY

    open_file () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_close_action ()
{
    NEMIVER_TRY

    close_current_file () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_execute_program_action ()
{
    NEMIVER_TRY

    execute_program () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_load_core_file_action ()
{
    NEMIVER_TRY

    load_core_file () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_attach_to_program_action ()
{
    NEMIVER_TRY

    attach_to_program () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_run_action ()
{
    NEMIVER_TRY

    run () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_stop_debugger_action (void)
{
    LOG_FUNCTION_SCOPE_NORMAL_D (NMV_DEFAULT_DOMAIN) ;
    NEMIVER_TRY

    stop () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_next_action ()
{
    NEMIVER_TRY

    step_over () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_step_into_action ()
{
    NEMIVER_TRY

    step_into () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_step_out_action ()
{
    NEMIVER_TRY

    step_out () ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_continue_action ()
{
    NEMIVER_TRY
    do_continue () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_toggle_breakpoint_action ()
{
    NEMIVER_TRY
    toggle_breakpoint () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_show_commands_action ()
{
    NEMIVER_TRY
    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (m_priv->workbench->get_ui_manager ()->get_action
                 ("/MenuBar/MenuBarAdditions/ViewMenu/ShowCommandsMenuItem")) ;
    THROW_IF_FAIL (action) ;

    set_show_command_view (action->get_active ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_show_errors_action ()
{
    NEMIVER_TRY

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (m_priv->workbench->get_ui_manager ()->get_action
                 ("/MenuBar/MenuBarAdditions/ViewMenu/ShowErrorsMenuItem")) ;
    THROW_IF_FAIL (action) ;

    set_show_log_view (action->get_active ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_show_target_output_action ()
{
    NEMIVER_TRY

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (m_priv->workbench->get_ui_manager ()->get_action
                 ("/MenuBar/MenuBarAdditions/ViewMenu/ShowTargetOutputMenuItem")) ;
    THROW_IF_FAIL (action) ;

    set_show_target_output_view (action->get_active ()) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_switch_page_signal (GtkNotebookPage *a_page, guint a_page_num)
{
    NEMIVER_TRY
    m_priv->current_page_num = a_page_num;
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_ready_signal (bool a_is_ready)
{
    NEMIVER_TRY

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->debugger_ready_action_group) ;
    THROW_IF_FAIL (m_priv->throbber) ;

    if (a_is_ready) {
        m_priv->throbber->stop () ;
        m_priv->debugger_ready_action_group->set_sensitive (true) ;
        m_priv->debugger_busy_action_group->set_sensitive (false) ;
        attached_to_target_signal ().emit (true) ;
    } else {
        m_priv->debugger_ready_action_group->set_sensitive (false) ;
        m_priv->debugger_busy_action_group->set_sensitive (true) ;
    }

    NEMIVER_CATCH
}


void
DBGPerspective::on_attached_to_target_signal (bool a_is_ready)
{
    NEMIVER_TRY

    if (a_is_ready) {
        m_priv->target_connected_action_group->set_sensitive (true) ;
    } else {
        m_priv->target_connected_action_group->set_sensitive (false) ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_insert_in_command_view_signal (const Gtk::TextBuffer::iterator &a_it,
                                                  const Glib::ustring &a_text,
                                                  int a_dont_know)
{
    NEMIVER_TRY
    if (a_text == "") {return;}

    if (a_text == "\n") {
        //get the command that is on the current line
        UString line ;
        Gtk::TextBuffer::iterator iter = a_it, tmp_iter, eol_iter = a_it ;
        for (;;) {
            --iter ;
            if (iter.is_start ()) {break;}
            tmp_iter = iter ;
            if (tmp_iter.get_char () == ')'
                && (--tmp_iter).get_char () == 'b'
                && (--tmp_iter).get_char () == 'd'
                && (--tmp_iter).get_char () == 'g'
                && (--tmp_iter).get_char () == '(') {
                ++ iter ;
                line = iter.get_visible_text (eol_iter) ;
                break ;
            }
        }
        if (!line.empty ()) {
            IDebuggerSafePtr dbg = debugger () ;
            THROW_IF_FAIL (dbg) ;
            dbg->execute_command (IDebugger::Command (line)) ;
            m_priv->last_command_text = "" ;
        }
    }
    NEMIVER_CATCH
}

void
DBGPerspective::on_source_view_markers_region_clicked_signal (int a_line)
{
    SourceEditor *cur_editor = get_current_source_editor () ;
    THROW_IF_FAIL (cur_editor) ;
    toggle_breakpoint (cur_editor->get_path (), a_line + 1 ) ;
}

bool
DBGPerspective::on_button_pressed_in_source_view_signal (GdkEventButton *a_event)
{
    NEMIVER_TRY

    if (a_event->type != GDK_BUTTON_PRESS) {
        return false ;
    }

    if (a_event->button != 3) {
        return false ;
    }
    popup_source_view_contextual_menu (a_event) ;

    NEMIVER_CATCH
    return true ;
}

bool
DBGPerspective::on_key_pressed_in_source_view_signal (GdkEventKey *a_event)
{
    if (a_event->type != GDK_KEY_PRESS) {
        return false ;
    }

    if (a_event->state == 0
        && a_event->keyval == GDK_F7){
        step_into () ;
        return true ;
    }

    if ((a_event->state & GDK_SHIFT_MASK)
        && a_event->keyval == GDK_F7){
        step_out () ;
        return true ;
    }

    if (a_event->state == 0
        && a_event->keyval == GDK_F8){
        step_over () ;
        return true ;
    }

    if (a_event->state == 0
        && a_event->keyval == GDK_F5){
        do_continue () ;
        return true ;
    }

    if ((a_event->state & GDK_SHIFT_MASK)
        && a_event->keyval == GDK_F5){
        run () ;
        return true ;
    }

    return false ;
}

void
DBGPerspective::on_shutdown_signal ()
{
    NEMIVER_TRY

    if (m_priv->prog_name == "") {
        return ;
    }

    if (m_priv->reused_session) {
        save_session (m_priv->session) ;
    } else {
        save_session () ;
    }

    NEMIVER_CATCH
}

void
DBGPerspective::on_show_command_view_changed_signal (bool a_show)
{
    Glib::RefPtr<Gtk::ToggleAction> action =
            Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
                (m_priv->workbench->get_ui_manager ()->get_action
                    ("/MenuBar/MenuBarAdditions/ViewMenu/ShowCommandsMenuItem"));
    THROW_IF_FAIL (action) ;
    action->set_active (a_show) ;
}

void
DBGPerspective::on_show_target_output_view_changed_signal (bool a_show)
{
    m_priv->target_output_view_is_visible = a_show ;

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (m_priv->workbench->get_ui_manager ()->get_action
                ("/MenuBar/MenuBarAdditions/ViewMenu/ShowTargetOutputMenuItem"));
    THROW_IF_FAIL (action) ;
    action->set_active (a_show) ;
}

void
DBGPerspective::on_show_log_view_changed_signal (bool a_show)
{
    m_priv->log_view_is_visible = a_show ;

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic
            (m_priv->workbench->get_ui_manager ()->get_action
                    ("/MenuBar/MenuBarAdditions/ViewMenu/ShowErrorsMenuItem"));
    THROW_IF_FAIL (action) ;

    action->set_active (a_show) ;
}

void
DBGPerspective::on_debugger_console_message_signal (const UString &a_msg)
{
    NEMIVER_TRY

    add_text_to_command_view (a_msg + "\n") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_target_output_message_signal
                                            (const UString &a_msg)
{
    NEMIVER_TRY

    add_text_to_target_output_view (a_msg + "\n") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_log_message_signal (const UString &a_msg)
{
    NEMIVER_TRY

    add_text_to_log_view (a_msg + "\n") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_command_done_signal (const UString &a_command)
{
    NEMIVER_TRY
    attached_to_target_signal ().emit (true) ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_breakpoints_set_signal
                                (const map<int, IDebugger::BreakPoint> &a_breaks)
{
    NEMIVER_TRY
    LOG_DD ("debugger engine set breakpoints") ;
    append_breakpoints (a_breaks) ;
    NEMIVER_CATCH
}


void
DBGPerspective::on_debugger_stopped_signal (const UString &a_reason,
                                            bool a_has_frame,
                                            const IDebugger::Frame &a_frame)
{
    NEMIVER_TRY

    if (a_has_frame
        && a_frame.file_full_name () == ""
        && a_frame.file_name () != "") {
        display_error ("Did not find file " + a_frame.file_name ()) ;
    }
    add_text_to_command_view ("\n(gdb)", true) ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_program_finished_signal ()
{
    NEMIVER_TRY

    unset_where () ;
    attached_to_target_signal ().emit (true) ;
    display_info ("Program exited") ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_frame_selected_signal (int a_index,
                                          const IDebugger::Frame &a_frame)
{
    NEMIVER_TRY

    if (a_frame.file_full_name () != ""
        && a_frame.line () != 0) {
        set_where (a_frame.file_full_name (), a_frame.line ()) ;
    } else if (a_frame.file_full_name () == "") {
        display_warning ("File path info is missing "
                         "for function '" + a_frame.function () + "'") ;
        //TODO: we should disassemble the current frame and display it.
    } else if (a_frame.line () == 0) {
        display_warning ("Line info is missing for function '"
                         + a_frame.function () + "'") ;
        //TODO: we should disassemble the current frame and display it.
    }
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_breakpoint_deleted_signal
                                        (const IDebugger::BreakPoint &a_break,
                                         int a_break_number)
{
    NEMIVER_TRY
    delete_visual_breakpoint (a_break_number) ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_running_signal ()
{
    NEMIVER_TRY
    THROW_IF_FAIL (m_priv->throbber) ;
    m_priv->throbber->start () ;
    NEMIVER_CATCH
}

void
DBGPerspective::on_signal_received_by_target_signal (const UString &a_signal,
                                                     const UString &a_meaning)
{
    NEMIVER_TRY

    ui_utils::display_info (_("Target received a signal : ")
                            + a_signal + ", " + a_meaning) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_error_signal (const UString &a_msg)
{
    NEMIVER_TRY

    ui_utils::display_error (_("An error occured: ") + a_msg) ;

    NEMIVER_CATCH
}

void
DBGPerspective::on_debugger_state_changed_signal (IDebugger::State a_state)
{
    NEMIVER_TRY

    if (a_state == IDebugger::READY) {
        debugger_ready_signal ().emit (true) ;
    } else {
        debugger_ready_signal ().emit (false) ;
    }

    NEMIVER_CATCH
}

//****************************
//</slots>
//***************************

//*******************
//<private methods>
//*******************


string
DBGPerspective::build_resource_path (const UString &a_dir, const UString &a_name)
{
    string relative_path = Glib::build_filename (Glib::locale_from_utf8 (a_dir),
                                                 Glib::locale_from_utf8 (a_name));
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;
    return absolute_path ;
}


void
DBGPerspective::add_stock_icon (const UString &a_stock_id,
                                const UString &a_icon_dir,
                                const UString &a_icon_name)
{
    if (!m_priv->icon_factory) {
        m_priv->icon_factory = Gtk::IconFactory::create () ;
        m_priv->icon_factory->add_default () ;
    }

    Gtk::StockID stock_id (a_stock_id) ;
    string icon_path = build_resource_path (a_icon_dir, a_icon_name) ;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf= Gdk::Pixbuf::create_from_file (icon_path) ;
    Gtk::IconSet icon_set (pixbuf) ;
    m_priv->icon_factory->add (stock_id, icon_set) ;
}

void
DBGPerspective::add_perspective_menu_entries ()
{
    string relative_path = Glib::build_filename ("menus",
                                                 "menus.xml") ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;

    m_priv->menubar_merge_id =
        m_priv->workbench->get_ui_manager ()->add_ui_from_file
                                        (Glib::locale_to_utf8 (absolute_path)) ;

    relative_path = Glib::build_filename ("menus", "contextualmenu.xml") ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;
    m_priv->contextual_menu_merge_id =
        m_priv->workbench->get_ui_manager ()->add_ui_from_file
                                        (Glib::locale_to_utf8 (absolute_path)) ;
}

void
DBGPerspective::init_perspective_menu_entries ()
{
    set_show_command_view (false) ;
    set_show_target_output_view (false) ;
    set_show_log_view (false) ;
    set_show_call_stack_view (true) ;
    set_show_variables_editor_view (true) ;
    set_show_terminal_view (true) ;
    m_priv->statuses_notebook->set_current_page (0) ;
}

void
DBGPerspective::add_perspective_toolbar_entries ()
{
    string relative_path = Glib::build_filename ("menus",
                                                 "toolbar.xml") ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;

    m_priv->toolbar_merge_id =
        m_priv->workbench->get_ui_manager ()->add_ui_from_file
                                            (Glib::locale_to_utf8 (absolute_path)) ;
}

void
DBGPerspective::init_icon_factory ()
{
    add_stock_icon (nemiver::SET_BREAKPOINT, "icons", "set-breakpoint.xpm") ;
    add_stock_icon (nemiver::CONTINUE, "icons", "continue.xpm") ;
    add_stock_icon (nemiver::STOP_DEBUGGER, "icons", "stop-debugger.xpm") ;
    add_stock_icon (nemiver::RUN_DEBUGGER, "icons", "run-debugger.xpm") ;
    add_stock_icon (nemiver::LINE_POINTER, "icons", "line-pointer.xpm") ;
    add_stock_icon (nemiver::RUN_TO_CURSOR, "icons", "run-to-cursor.xpm") ;
    add_stock_icon (nemiver::STEP_INTO, "icons", "step-into.xpm") ;
    add_stock_icon (nemiver::STEP_OVER, "icons", "step-over.xpm") ;
    add_stock_icon (nemiver::STEP_OUT, "icons", "step-out.xpm") ;
}

void
DBGPerspective::init_actions ()
{
    Gtk::StockID nil_stock_id ("") ;
    sigc::slot<void> nil_slot ;
    static ui_utils::ActionEntry s_target_connected_action_entries [] = {
        {
            "RunMenuItemAction",
            nemiver::STOCK_RUN_DEBUGGER,
            _("_Run"),
            _("Run the debugger starting from program's begining"),
            sigc::mem_fun (*this, &DBGPerspective::on_run_action),
            ActionEntry::DEFAULT,
            "<shift>F5"
        }
    };

    static ui_utils::ActionEntry s_debugger_ready_action_entries [] = {
        {
            "NextMenuItemAction",
            nemiver::STOCK_STEP_OVER,
            _("_Next"),
            _("Execute next instruction steping over the next function, if any"),
            sigc::mem_fun (*this, &DBGPerspective::on_next_action),
            ActionEntry::DEFAULT,
            "F6"
        }
        ,
        {
            "StepMenuItemAction",
            nemiver::STOCK_STEP_INTO,
            _("_Step"),
            _("Execute next instruction, steping into the next function, if any"),
            sigc::mem_fun (*this, &DBGPerspective::on_step_into_action),
            ActionEntry::DEFAULT,
            "F7"
        }
        ,
        {
            "StepOutMenuItemAction",
            nemiver::STOCK_STEP_OUT,
            _("Step _out"),
            _("Finish the execution of the current function"),
            sigc::mem_fun (*this, &DBGPerspective::on_step_out_action),
            ActionEntry::DEFAULT,
            "<shift>F7"
        }
        ,
        {
            "ContinueMenuItemAction",
            nemiver::STOCK_CONTINUE,
            _("_Continue"),
            _("Continue program execution until the next breakpoint"),
            sigc::mem_fun (*this, &DBGPerspective::on_continue_action),
            ActionEntry::DEFAULT,
            "F5"
        }
        ,
        {
            "ToggleBreakPointMenuItemAction",
            nemiver::STOCK_SET_BREAKPOINT,
            _("Toggle _break"),
            _("Set/Unset a breakpoint at the current cursor location"),
            sigc::mem_fun (*this, &DBGPerspective::on_toggle_breakpoint_action),
            ActionEntry::DEFAULT,
            "F8"
        }
    };

    static ui_utils::ActionEntry s_debugger_busy_action_entries [] = {
        {
            "StopMenuItemAction",
            nil_stock_id,
            _("Stop"),
            _("Stop the debugger"),
            sigc::mem_fun (*this, &DBGPerspective::on_stop_debugger_action),
            ActionEntry::DEFAULT,
            "F9"
        }
    };

    static ui_utils::ActionEntry s_default_action_entries [] = {
        {
            "ViewMenuAction",
            nil_stock_id,
            _("_View"),
            "",
            nil_slot,
            ActionEntry::DEFAULT
        },
        {
            "ShowCommandsMenuAction",
            nil_stock_id,
            _("Show commands"),
            _("Show the debugger commands tab"),
            sigc::mem_fun (*this, &DBGPerspective::on_show_commands_action),
            ActionEntry::TOGGLE
        },
        {
            "ShowErrorsMenuAction",
            nil_stock_id,
            _("Show errors"),
            _("Show the errors commands tab"),
            sigc::mem_fun (*this, &DBGPerspective::on_show_errors_action),
            ActionEntry::TOGGLE
        },
        {
            "ShowTargetOutputMenuAction",
            nil_stock_id,
            _("Show output"),
            _("Show the debugged target output tab"),
            sigc::mem_fun (*this, &DBGPerspective::on_show_target_output_action),
            ActionEntry::TOGGLE
        },
        {
            "DebugMenuAction",
            nil_stock_id,
            _("_Debug"),
            "",
            nil_slot,
            ActionEntry::DEFAULT
        }
        ,
        {
            "OpenMenuItemAction",
            Gtk::Stock::OPEN,
            _("_Open"),
            _("Open a file"),
            sigc::mem_fun (*this, &DBGPerspective::on_open_action),
            ActionEntry::DEFAULT
        },
        {
            "ExecuteProgramMenuItemAction",
            Gtk::Stock::EXECUTE,
            _("_Execute..."),
            _("Execute a program"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_execute_program_action),
            ActionEntry::DEFAULT
        },
        {
            "LoadCoreMenuItemAction",
            nil_stock_id,
            _("_Load core file..."),
            _("Load a core file from disk"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_load_core_file_action),
            ActionEntry::DEFAULT
        },
        {
            "AttachToProgramMenuItemAction",
            nil_stock_id,
            _("_Attach to running program..."),
            _("Debug a program that's already running"),
            sigc::mem_fun (*this,
                           &DBGPerspective::on_attach_to_program_action),
            ActionEntry::DEFAULT
        }
    };

    static ui_utils::ActionEntry s_file_opened_action_entries [] = {
        {
            "CloseMenuItemAction",
            Gtk::Stock::CLOSE,
            _("_Close"),
            _("Close the opened file"),
            sigc::mem_fun (*this, &DBGPerspective::on_close_action),
            ActionEntry::DEFAULT
        }
    };

    m_priv->target_connected_action_group =
                Gtk::ActionGroup::create ("target-connected-action-group") ;
    m_priv->target_connected_action_group->set_sensitive (false) ;

    m_priv->debugger_ready_action_group =
                Gtk::ActionGroup::create ("debugger-ready-action-group") ;
    m_priv->debugger_ready_action_group->set_sensitive (false) ;

    m_priv->debugger_busy_action_group =
                Gtk::ActionGroup::create ("debugger-busy-action-group") ;
    m_priv->debugger_busy_action_group->set_sensitive (false) ;

    m_priv->default_action_group =
                Gtk::ActionGroup::create ("debugger-default-action-group") ;
    m_priv->default_action_group->set_sensitive (true) ;

    m_priv->opened_file_action_group =
                Gtk::ActionGroup::create ("opened-file-action-group") ;
    m_priv->opened_file_action_group->set_sensitive (false) ;

    int num_actions =
     sizeof (s_target_connected_action_entries)/sizeof (ui_utils::ActionEntry);
    ui_utils::add_action_entries_to_action_group
                        (s_target_connected_action_entries,
                         num_actions,
                         m_priv->target_connected_action_group) ;

    num_actions =
         sizeof (s_debugger_ready_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_debugger_ready_action_entries,
                         num_actions,
                         m_priv->debugger_ready_action_group) ;

    num_actions =
         sizeof (s_debugger_busy_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_debugger_busy_action_entries,
                         num_actions,
                         m_priv->debugger_busy_action_group) ;

    num_actions =
         sizeof (s_default_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_default_action_entries,
                         num_actions,
                         m_priv->default_action_group) ;

    num_actions =
         sizeof (s_file_opened_action_entries)/sizeof (ui_utils::ActionEntry) ;

    ui_utils::add_action_entries_to_action_group
                        (s_file_opened_action_entries,
                         num_actions,
                         m_priv->opened_file_action_group) ;

    m_priv->workbench->get_ui_manager ()->insert_action_group
                                        (m_priv->target_connected_action_group) ;
    m_priv->workbench->get_ui_manager ()->insert_action_group
                                            (m_priv->debugger_busy_action_group) ;
    m_priv->workbench->get_ui_manager ()->insert_action_group
                                            (m_priv->debugger_ready_action_group);
    m_priv->workbench->get_ui_manager ()->insert_action_group
                                            (m_priv->default_action_group);
    m_priv->workbench->get_ui_manager ()->insert_action_group
                                            (m_priv->opened_file_action_group);

    m_priv->workbench->get_root_window ().add_accel_group
        (m_priv->workbench->get_ui_manager ()->get_accel_group ()) ;
}


void
DBGPerspective::init_toolbar ()
{
    add_perspective_toolbar_entries () ;

    m_priv->throbber = Throbber::create (plugin_path ()) ;
    m_priv->toolbar = new Gtk::HBox ;
    THROW_IF_FAIL (m_priv->toolbar) ;
    m_priv->toolbar->pack_end (m_priv->throbber->get_widget (), Gtk::PACK_SHRINK) ;
    Gtk::Toolbar *glade_toolbar = dynamic_cast<Gtk::Toolbar*>
            (m_priv->workbench->get_ui_manager ()->get_widget ("/ToolBar")) ;
    THROW_IF_FAIL (glade_toolbar) ;
    m_priv->toolbar->pack_start (*glade_toolbar) ;
    m_priv->toolbar->show_all () ;

    Gtk::ToolButton *button=NULL ;

    button = dynamic_cast<Gtk::ToolButton*>
    (m_priv->workbench->get_ui_manager ()->get_widget ("/ToolBar/RunToolItem")) ;
    THROW_IF_FAIL (button) ;
}

void
DBGPerspective::init_body ()
{
    string relative_path = Glib::build_filename ("glade",
                                                 "bodycontainer.glade") ;
    string absolute_path ;
    THROW_IF_FAIL (build_absolute_resource_path
                    (Glib::locale_to_utf8 (relative_path),
                                           absolute_path)) ;
    m_priv->body_glade = Gnome::Glade::Xml::create (absolute_path) ;
    m_priv->body_window =
        env::get_widget_from_glade<Gtk::Window> (m_priv->body_glade,
                                                 "bodycontainer") ;
    Glib::RefPtr<Gtk::Paned> paned
        (env::get_widget_from_glade<Gtk::Paned> (m_priv->body_glade,
                                                 "mainbodypaned")) ;
    paned->reference () ;
    m_priv->body_main_paned = paned ;

    m_priv->sourceviews_notebook =
        env::get_widget_from_glade<Gtk::Notebook> (m_priv->body_glade,
                                                   "sourceviewsnotebook") ;
    m_priv->sourceviews_notebook->remove_page () ;
    m_priv->sourceviews_notebook->set_show_tabs () ;
    m_priv->sourceviews_notebook->set_scrollable () ;

    m_priv->statuses_notebook =
        env::get_widget_from_glade<Gtk::Notebook> (m_priv->body_glade,
                                                   "statusesnotebook") ;
    m_priv->command_view = new Gtk::TextView ;
    THROW_IF_FAIL (m_priv->command_view) ;
    get_command_view_scrolled_win ().add (*m_priv->command_view) ;
    m_priv->command_view->set_editable (true) ;
    m_priv->command_view->get_buffer ()->signal_insert ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_insert_in_command_view_signal)) ;

    m_priv->target_output_view = new Gtk::TextView;
    THROW_IF_FAIL (m_priv->target_output_view) ;
    get_target_output_view_scrolled_win ().add (*m_priv->target_output_view) ;
    m_priv->target_output_view->set_editable (false) ;

    m_priv->log_view = new Gtk::TextView ;
    get_log_view_scrolled_win ().add (*m_priv->log_view) ;
    m_priv->log_view->set_editable (false) ;

    get_call_stack_scrolled_win ().add (get_call_stack ().widget ()) ;
    get_variables_editor_scrolled_win ().add (get_variables_editor ().widget ());

    get_terminal_scrolled_win ().add (get_terminal ().widget ()) ;

    /*
    set_show_call_stack_view (true) ;
    set_show_variables_editor_view (true) ;
    */

    m_priv->body_main_paned->unparent () ;
    m_priv->body_main_paned->show_all () ;

    //must be last
    init_perspective_menu_entries () ;
}

void
DBGPerspective::init_signals ()
{
    m_priv->sourceviews_notebook->signal_switch_page ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_switch_page_signal)) ;
    debugger_ready_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_ready_signal)) ;
    attached_to_target_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_attached_to_target_signal)) ;
    show_command_view_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_show_command_view_changed_signal)) ;
    show_target_output_view_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_show_target_output_view_changed_signal));
    show_log_view_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_show_log_view_changed_signal));
    get_call_stack ().frame_selected_signal ().connect
        (sigc::mem_fun (*this, &DBGPerspective::on_frame_selected_signal));

}

void
DBGPerspective::init_debugger_signals ()
{
    debugger ()->console_message_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_console_message_signal)) ;

    debugger ()->target_output_message_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_target_output_message_signal)) ;

    debugger ()->log_message_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_log_message_signal)) ;

    debugger ()->command_done_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_command_done_signal)) ;

    debugger ()->breakpoints_set_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_breakpoints_set_signal)) ;

    debugger ()->breakpoint_deleted_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_breakpoint_deleted_signal)) ;

    debugger ()->stopped_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_stopped_signal)) ;

    debugger ()->program_finished_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_program_finished_signal)) ;

    debugger ()->running_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_running_signal)) ;

    debugger ()->signal_received_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_signal_received_by_target_signal)) ;

    debugger ()->error_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_error_signal)) ;

    debugger ()->state_changed_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_debugger_state_changed_signal)) ;
}


void
DBGPerspective::append_source_editor (SourceEditor &a_sv,
                                      const UString &a_path)
{
    if (a_path == "") {return;}

    if (m_priv->path_2_pagenum_map.find (a_path)
        != m_priv->path_2_pagenum_map.end ()) {
        THROW (UString ("File of '") + a_path + "' is already loaded") ;
    }

    UString basename = Glib::locale_to_utf8
        (Glib::path_get_basename (Glib::locale_from_utf8 (a_path))) ;

    SafePtr<Gtk::Label> label (Gtk::manage
                            (new Gtk::Label (basename))) ;
    SafePtr<Gtk::Image> cicon (manage
                (new Gtk::Image (Gtk::StockID ("gtk-close"),
                                               Gtk::ICON_SIZE_BUTTON))) ;

    int w=0, h=0 ;
    Gtk::IconSize::lookup (Gtk::ICON_SIZE_MENU, w, h) ;
    SafePtr<SlotedButton> close_button (Gtk::manage (new SlotedButton ())) ;
    close_button->perspective = this ;
    close_button->set_size_request (w+4, h+4) ;
    close_button->set_relief (Gtk::RELIEF_NONE) ;
    close_button->add (*cicon) ;
    close_button->file_path = a_path ;
    close_button->signal_clicked ().connect
            (sigc::mem_fun (*close_button, &SlotedButton::on_clicked)) ;

    SafePtr<Gtk::Table> table (Gtk::manage (new Gtk::Table (1, 2))) ;
    table->attach (*label, 0, 1, 0, 1) ;
    table->attach (*close_button, 1, 2, 0, 1) ;

    table->show_all () ;
    int page_num = m_priv->sourceviews_notebook->insert_page (a_sv,
                                                              *table,
                                                              -1);
    m_priv->path_2_pagenum_map[a_path] = page_num ;
    m_priv->pagenum_2_source_editor_map[page_num] = &a_sv;
    m_priv->pagenum_2_path_map[page_num] = a_path ;

    table.release () ;
    close_button.release () ;
    label.release () ;
    cicon.release () ;
}

SourceEditor*
DBGPerspective::get_current_source_editor ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->sourceviews_notebook) {return NULL;}

    if (m_priv->sourceviews_notebook
        && !m_priv->sourceviews_notebook->get_n_pages ()) {
        return NULL ;
    }

    map<int, SourceEditor*>::iterator iter, nil ;
    nil = m_priv->pagenum_2_source_editor_map.end () ;

    iter = m_priv->pagenum_2_source_editor_map.find (m_priv->current_page_num) ;
    if (iter == nil) {return NULL ;}

    return iter->second ;
}

ISessMgr*
DBGPerspective::get_session_manager ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->session_manager) {
        m_priv->session_manager = ISessMgr::create (plugin_path ()) ;
        THROW_IF_FAIL (m_priv->session_manager) ;
    }
    return m_priv->session_manager.get () ;
}

UString
DBGPerspective::get_current_file_path ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    if (!source_editor) {return "";}
    return source_editor->get_path () ;
}

SourceEditor*
DBGPerspective::get_source_editor_from_path (const UString &a_path)
{
    map<UString, int>::iterator iter =
        m_priv->path_2_pagenum_map.find (a_path) ;
    if (iter == m_priv->path_2_pagenum_map.end ()) {
        return NULL ;
    }
    return m_priv->pagenum_2_source_editor_map[iter->second] ;
}

void
DBGPerspective::bring_source_as_current (const UString &a_path)
{
    SourceEditor *source_editor = get_source_editor_from_path (a_path) ;
    if (!source_editor) {
        open_file (a_path) ;
    }
    source_editor = get_source_editor_from_path (a_path) ;
    THROW_IF_FAIL (source_editor) ;
    map<UString, int>::iterator iter =
        m_priv->path_2_pagenum_map.find (a_path) ;
    THROW_IF_FAIL (iter != m_priv->path_2_pagenum_map.end ()) ;
    m_priv->sourceviews_notebook->set_current_page (iter->second) ;
}

void
DBGPerspective::set_where (const UString &a_path,
                           int a_line)
{
    bring_source_as_current (a_path) ;
    SourceEditor *source_editor = get_source_editor_from_path (a_path) ;
    THROW_IF_FAIL (source_editor) ;
    source_editor->move_where_marker_to_line (a_line) ;
    Gtk::TextBuffer::iterator iter =
        source_editor->source_view().get_buffer ()->get_iter_at_line (a_line-1) ;
    if (!iter) {return;}
        source_editor->source_view().get_buffer ()->place_cursor (iter) ;
}

void
DBGPerspective::unset_where ()
{
    map<int, SourceEditor*>::iterator iter ;
    for (iter = m_priv->pagenum_2_source_editor_map.begin ();
         iter !=m_priv->pagenum_2_source_editor_map.end ();
         ++iter) {
        iter->second->unset_where_marker () ;
    }
}

Gtk::Widget*
DBGPerspective::get_contextual_menu ()
{
    THROW_IF_FAIL (m_priv && m_priv->contextual_menu_merge_id) ;

    if (!m_priv->contextual_menu) {

        m_priv->workbench->get_ui_manager ()->add_ui
            (m_priv->contextual_menu_merge_id,
             "/ContextualMenu",
             "ToggleBreakPointMenuItem",
             "ToggleBreakPointMenuItemAction") ;

        m_priv->workbench->get_ui_manager ()->ensure_update () ;
        m_priv->contextual_menu =
            m_priv->workbench->get_ui_manager ()->get_widget
            ("/ContextualMenu") ;
        THROW_IF_FAIL (m_priv->contextual_menu) ;
    }
    return m_priv->contextual_menu ;
}

int
DBGPerspective::get_n_pages ()
{
    THROW_IF_FAIL (m_priv && m_priv->sourceviews_notebook) ;

    return m_priv->sourceviews_notebook->get_n_pages () ;
}

void
DBGPerspective::popup_source_view_contextual_menu (GdkEventButton *a_event)
{
    SourceEditor *editor = get_current_source_editor () ;
    THROW_IF_FAIL (editor) ;
    int buffer_x=0, buffer_y=0, line_top=0;
    Gtk::TextBuffer::iterator cur_iter ;
    UString file_name ;

    editor->source_view ().window_to_buffer_coords (Gtk::TEXT_WINDOW_TEXT,
                                                    (int)a_event->x,
                                                    (int)a_event->y,
                                                    buffer_x, buffer_y) ;
    editor->source_view ().get_line_at_y (cur_iter, buffer_y, line_top) ;

    file_name = editor->get_path () ;

    Gtk::Menu *menu = dynamic_cast<Gtk::Menu*> (get_contextual_menu ()) ;
    THROW_IF_FAIL (menu) ;
    editor->source_view ().get_buffer ()->place_cursor (cur_iter) ;
    menu->popup (a_event->button, a_event->time) ;
}

void
DBGPerspective::save_session ()
{
    THROW_IF_FAIL (m_priv) ;
    ISessMgr::Session session ;
    save_session (session) ;
}

IProcMgr*
DBGPerspective::get_process_manager ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->process_manager) {
        m_priv->process_manager = IProcMgr::create () ;
        THROW_IF_FAIL (m_priv->process_manager) ;
    }
    return m_priv->process_manager.get () ;
}

void
DBGPerspective::save_session (ISessMgr::Session &a_session)
{
    THROW_IF_FAIL (m_priv) ;
    ISessMgr::Session session ;
    UString session_name = m_priv->prog_name ;
    if (session_name == "") {return;}

    UString today ;
    dateutils::get_current_datetime (today) ;
    session_name += "-" + today ;

    a_session.properties ()[SESSION_NAME] = session_name ;
    a_session.properties ()[PROGRAM_NAME] = m_priv->prog_name ;
    a_session.properties ()[PROGRAM_ARGS] = m_priv->prog_args ;
    a_session.properties ()[PROGRAM_CWD] = m_priv->prog_cwd ;

    map<UString, int>::const_iterator path_iter =
        m_priv->path_2_pagenum_map.begin () ;
    for (;
         path_iter != m_priv->path_2_pagenum_map.end ();
         ++path_iter) {
        a_session.opened_files ().push_back (path_iter->first) ;
    }

    map<int, IDebugger::BreakPoint>::const_iterator break_iter ;
    for (break_iter = m_priv->breakpoints.begin ();
         break_iter != m_priv->breakpoints.end ();
         ++break_iter) {
        a_session.breakpoints ().push_back
            (ISessMgr::BreakPoint (break_iter->second.full_file_name (),
                                   break_iter->second.line ())) ;
    }
    THROW_IF_FAIL (get_session_manager ()) ;
    get_session_manager ()->store_session
                            (a_session,
                             get_session_manager ()->default_transaction ()) ;
}


//*******************
//</private methods>
//*******************

DBGPerspective::DBGPerspective ()
{
    m_priv = new Priv ;
}

void
DBGPerspective::get_info (Info &a_info) const
{
    static Info s_info ("Debugger perspective plugin",
                        "The debugger perspective of Nemiver",
                        "1.0") ;
    a_info = s_info ;
}

void
DBGPerspective::do_init ()
{
}

void
DBGPerspective::do_init (IWorkbenchSafePtr &a_workbench)
{
    THROW_IF_FAIL (m_priv) ;
    m_priv->workbench = a_workbench ;
    init_icon_factory () ;
    init_actions () ;
    init_toolbar () ;
    init_body () ;
    init_signals () ;
    init_debugger_signals () ;
    m_priv->initialized = true ;
    session_manager ().load_sessions (session_manager ().default_transaction ());
    m_priv->workbench->shutting_down_signal ().connect (sigc::mem_fun
            (*this, &DBGPerspective::on_shutdown_signal)) ;
}

DBGPerspective::~DBGPerspective ()
{
    m_priv = NULL ;
}

const UString&
DBGPerspective::get_perspective_identifier ()
{
    static UString s_id = "org.nemiver.DebuggerPerspective" ;
    return s_id ;
}

void
DBGPerspective::get_toolbars (list<Gtk::Widget*>  &a_tbs)
{
    CHECK_P_INIT ;
    a_tbs.push_back (m_priv->toolbar.get ()) ;
}

Gtk::Widget*
DBGPerspective::get_body ()
{
    CHECK_P_INIT ;
    return m_priv->body_main_paned.operator->() ;
}

void
DBGPerspective::edit_workbench_menu ()
{
    CHECK_P_INIT ;

    add_perspective_menu_entries () ;
    //init_perspective_menu_entries () ;
}

void
DBGPerspective::open_file ()
{
    Gtk::FileChooserDialog file_chooser (_("Open file"),
                                         Gtk::FILE_CHOOSER_ACTION_OPEN) ;

    file_chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL) ;
    file_chooser.add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK) ;
    file_chooser.set_select_multiple (true) ;

    int result = file_chooser.run () ;

    if (result != Gtk::RESPONSE_OK) {return;}

    list<UString> paths = file_chooser.get_filenames () ;
    list<UString>::const_iterator iter ;

    for (iter = paths.begin () ; iter != paths.end () ; ++iter) {
        open_file (*iter) ;
    }
}


bool
DBGPerspective::open_file (const UString &a_path,
                           int a_current_line)
{
    if (a_path == "") {return false;}
    if (get_source_editor_from_path (a_path)) {return true ;}

    ifstream file (Glib::locale_from_utf8 (a_path).c_str ()) ;
    if (!file.good () && !file.eof ()) {
        LOG_ERROR ("Could not open file " + a_path) ;
        ui_utils::display_error ("Could not open file: " + a_path) ;
        return false ;
    }

    NEMIVER_TRY

    UString base_name = Glib::locale_to_utf8
        (Glib::path_get_basename (Glib::locale_from_utf8 (a_path))) ;

    UString mime_type = gnome_vfs_get_mime_type_for_name (base_name.c_str ()) ;
    if (mime_type == "") {
        mime_type = "text/x-c++" ;
    }

    Glib::RefPtr<SourceLanguagesManager> lang_manager =
                                    SourceLanguagesManager::create () ;
    Glib::RefPtr<SourceLanguage> lang =
        lang_manager->get_language_from_mime_type (mime_type) ;

    Glib::RefPtr<SourceBuffer> source_buffer = SourceBuffer::create (lang) ;
    THROW_IF_FAIL (source_buffer) ;

    gint buf_size = 10 * 1024 ;
    SafePtr<gchar> buf (new gchar [buf_size + 1]) ;

    for (;;) {
        file.read (buf.get (), buf_size) ;
        THROW_IF_FAIL (file.good () || file.eof ()) ;
        source_buffer->insert (source_buffer->end (), buf.get (),
                               buf.get () + file.gcount ()) ;
        if (file.gcount () != buf_size) {break;}
    }
    file.close () ;

    source_buffer->set_highlight () ;
    SourceEditor *source_editor (Gtk::manage
                                (new SourceEditor (plugin_path (),
                                 source_buffer)));
    source_editor->set_path (a_path) ;
    source_editor->marker_region_got_clicked_signal ().connect
        (sigc::mem_fun
                 (*this,
                  &DBGPerspective::on_source_view_markers_region_clicked_signal));

    if (a_current_line > 0) {
        Gtk::TextIter cur_line_iter =
                source_buffer->get_iter_at_line (a_current_line) ;
        if (cur_line_iter) {
            Glib::RefPtr<SourceMarker> where_marker =
                source_buffer->create_marker ("where-marker",
                                              "line-pointer-marker",
                                              cur_line_iter) ;
            THROW_IF_FAIL (where_marker) ;
        }
    }
    source_editor->show_all () ;
    append_source_editor (*source_editor, a_path) ;

    if (!source_editor->source_view ().has_no_window ()) {
        source_editor->source_view ().add_events
                        (Gdk::BUTTON3_MOTION_MASK |Gdk::KEY_PRESS_MASK) ;
        source_editor->source_view ().signal_button_press_event ().connect
            (sigc::mem_fun
             (*this,
              &DBGPerspective::on_button_pressed_in_source_view_signal)) ;
    }

    m_priv->opened_file_action_group->set_sensitive (true) ;

    NEMIVER_CATCH_AND_RETURN (false);
    return true ;
}

void
DBGPerspective::close_current_file ()
{
    if (!get_n_pages ()) {return;}

    close_file (m_priv->pagenum_2_path_map[m_priv->current_page_num]) ;
}

void
DBGPerspective::close_file (const UString &a_path)
{
    map<UString, int>::const_iterator nil, iter ;
    nil = m_priv->path_2_pagenum_map.end () ;
    iter = m_priv->path_2_pagenum_map.find (a_path) ;
    if (iter == nil) {return;}

    int page_num = m_priv->path_2_pagenum_map[a_path] ;
    m_priv->sourceviews_notebook->remove_page (page_num) ;
    m_priv->path_2_pagenum_map.erase (a_path) ;
    m_priv->pagenum_2_source_editor_map.erase (page_num) ;
    m_priv->pagenum_2_path_map.erase (page_num) ;

    if (!get_n_pages ()) {
        m_priv->opened_file_action_group->set_sensitive (false) ;
    }
}

ISessMgr&
DBGPerspective::session_manager ()
{
    return *get_session_manager () ;
}

void
DBGPerspective::execute_session (ISessMgr::Session &a_session)
{
    m_priv->session = a_session ;
    map<UString, UString> env ;//TODO: get env from session
    execute_program (a_session.properties ()[PROGRAM_NAME],
                     a_session.properties ()[PROGRAM_ARGS],
                     env,
                     a_session.properties ()[PROGRAM_CWD]) ;
    m_priv->reused_session = true ;
}

void
DBGPerspective::execute_program ()
{
    RunProgramDialog dialog (plugin_path ()) ;

    int result = dialog.run () ;
    if (result != Gtk::RESPONSE_OK) {
        return;
    }

    UString prog, args, cwd ;
    prog = dialog.program_name () ;
    THROW_IF_FAIL (prog != "") ;
    args = dialog.arguments () ;
    cwd = dialog.working_directory () ;
    THROW_IF_FAIL (cwd != "") ;

    map<UString, UString> env = dialog.environment_variables();
    execute_program (prog, args, env, cwd) ;
    m_priv->reused_session = false ;
}

void
DBGPerspective::execute_program (const UString &a_prog_and_args,
                                 const map<UString, UString> &a_env,
                                 const UString &a_cwd)
{
    vector<UString> argv = a_prog_and_args.split (" ") ;
    vector<UString>::const_iterator iter = argv.begin () ;
    vector<UString>::const_iterator end_iter = argv.end () ;
    ++iter ;
    UString prog_name=argv[0], args = UString::join (iter, end_iter);
    execute_program (prog_name, args, a_env, a_cwd) ;
    m_priv->reused_session = false ;
}

void
DBGPerspective::execute_program (const UString &a_prog,
                                 const UString &a_args,
                                 const map<UString, UString> &a_env,
                                 const UString &a_cwd)
{
    NEMIVER_TRY

    m_priv->debugger->properties ().operator=
                                    (m_priv->workbench->get_properties ()) ;
    IDebuggerSafePtr dbg_engine = debugger () ;
    THROW_IF_FAIL (dbg_engine) ;
    vector<UString> args = a_args.split (" ") ;
    args.insert (args.begin (), a_prog) ;
    vector<UString> source_search_dirs = a_cwd.split (" ") ;

    dbg_engine->load_program (args, source_search_dirs,
                              get_terminal ().slave_pts_name ()) ;

    dbg_engine->add_env_variables (a_env) ;

    dbg_engine->set_breakpoint ("main") ;

    dbg_engine->run () ;

    attached_to_target_signal ().emit (true) ;

    m_priv->prog_name = a_prog ;
    m_priv->prog_args = a_args ;
    m_priv->prog_cwd = a_cwd ;

    NEMIVER_CATCH
}

void
DBGPerspective::attach_to_program ()
{
    IProcMgr *process_manager = get_process_manager () ;
    THROW_IF_FAIL (process_manager) ;
    ProcListDialog dialog (plugin_path (),
                           *process_manager);
    int result = dialog.run () ;
    if (result != Gtk::RESPONSE_OK) {
        return;
    }
    if (dialog.has_selected_process ()) {
        IProcMgr::Process process ;
        THROW_IF_FAIL (dialog.get_selected_process (process));
        attach_to_program (process.pid ()) ;
    }
}

void
DBGPerspective::attach_to_program (unsigned int a_pid)
{
    if (a_pid == (unsigned int) getpid ()) {
        ui_utils::display_warning ("You can not attach to nemiver itself") ;
        return ;
    }
    if (!debugger ()->attach_to_program (a_pid,
                                         get_terminal ().slave_pts_name ())) {
        ui_utils::display_warning ("You can not attach to the "
                                   "underlying debugger engine") ;
    }
}

void
DBGPerspective::run ()
{
    debugger ()->run () ;
}

void
DBGPerspective::load_core_file ()
{
    LoadCoreDialog dialog (plugin_path ()) ;

    int result = dialog.run () ;
    if (result != Gtk::RESPONSE_OK) {
        return;
    }

    UString prog_path, core_path ;
    prog_path = dialog.program_name () ;
    THROW_IF_FAIL (prog_path != "") ;
    core_path = dialog.core_file () ;
    THROW_IF_FAIL (core_path != "") ;

    load_core_file (prog_path, core_path) ;
}

void
DBGPerspective::load_core_file (const UString &a_prog_path,
                                const UString &a_core_file_path)
{
    debugger ()->load_core_file (a_prog_path, a_core_file_path) ;
    debugger ()->list_frames () ;
}

void
DBGPerspective::stop ()
{
    LOG_FUNCTION_SCOPE_NORMAL_D (NMV_DEFAULT_DOMAIN) ;
    if (!debugger ()->stop ()) {
        ui_utils::display_error ("Failed to stop the debugger") ;
    }
}

void
DBGPerspective::step_over ()
{
    debugger ()->step_over () ;
}

void
DBGPerspective::step_into ()
{
    debugger ()->step_in () ;
}

void
DBGPerspective::step_out ()
{
    debugger ()->step_out () ;
}

void
DBGPerspective::do_continue ()
{
    debugger ()->do_continue () ;
}

void
DBGPerspective::set_breakpoint ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    THROW_IF_FAIL (source_editor->get_path () != "") ;

    //line numbers start at 0 in GtkSourceView, but at 1 in GDB <grin/>
    //so in DGBPerspective, the line number are set in the GDB's reference.

    gint current_line =
        source_editor->source_view ().get_source_buffer ()->get_insert
            ()->get_iter ().get_line () + 1;
    set_breakpoint (source_editor->get_path (), current_line) ;
}

void
DBGPerspective::set_breakpoint (const UString &a_file_path,
                                int a_line)
{
    LOG_DD ("set bkpoint request for " << a_file_path << ":" << a_line) ;
    debugger ()->set_breakpoint (a_file_path, a_line) ;
}

void
DBGPerspective::append_breakpoints
                    (const map<int, IDebugger::BreakPoint> &a_breaks)
{
    map<int, IDebugger::BreakPoint>::const_iterator iter ;
    for (iter = a_breaks.begin () ; iter != a_breaks.end () ; ++iter) {
        LOG_DD ("record breakpoint "
                << iter->second.full_file_name ()
                << ":" << iter->second.line ()) ;
        m_priv->breakpoints[iter->first] = iter->second ;
        LOG_DD ("append visual breakpoint at source view line: "
                << iter->second.full_file_name () << ":"
                << iter->second.line () - 1) ;
        append_visual_breakpoint (iter->second.full_file_name (),
                                  iter->second.line () - 1) ;
    }
}

bool
DBGPerspective::get_breakpoint_number (const UString &a_file_name,
                                       int a_line_num,
                                       int &a_break_num)
{
    UString breakpoint = a_file_name + ":" + UString::from_int (a_line_num) ;

    LOG_DD ("searching for breakpoint " << breakpoint << ": ") ;

    map<int, IDebugger::BreakPoint>::const_iterator iter ;
    for (iter = m_priv->breakpoints.begin () ;
         iter != m_priv->breakpoints.end () ;
         ++iter) {
        LOG_DD ("got breakpoint " << iter->second.full_file_name ()
                << ":" << iter->second.line () << "...") ;
        if (   (iter->second.full_file_name () == a_file_name)
            && (iter->second.line () == a_line_num)) {
            a_break_num= iter->second.number () ;
            LOG_DD ("found breakpoint " << breakpoint << " !") ;
            return true ;
        }
    }
    LOG_DD ("did not find breakpoint " + breakpoint) ;
    return false ;
}

bool
DBGPerspective::delete_breakpoint ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    THROW_IF_FAIL (source_editor->get_path () != "") ;

    gint current_line =
        source_editor->source_view ().get_source_buffer ()->get_insert
            ()->get_iter ().get_line () + 1;
    int break_num=0 ;
    if (!get_breakpoint_number (source_editor->get_path (),
                                current_line,
                                break_num)) {
        return false ;
    }
    THROW_IF_FAIL (break_num) ;
    return delete_breakpoint (break_num) ;
}

bool
DBGPerspective::delete_breakpoint (int a_breakpoint_num)
{
    map<int, IDebugger::BreakPoint>::iterator iter =
        m_priv->breakpoints.find (a_breakpoint_num) ;
    if (iter == m_priv->breakpoints.end ()) {
        LOG_ERROR ("breakpoint " << (int) a_breakpoint_num << " not found") ;
        return false ;
    }
    debugger ()->delete_breakpoint (a_breakpoint_num) ;
    return true ;
}

void
DBGPerspective::append_visual_breakpoint (const UString &a_file_name,
                                          int a_linenum)
{
    if (a_linenum < 0) {a_linenum = 0;}

    SourceEditor *source_editor = get_source_editor_from_path (a_file_name) ;
    if (!source_editor) {
        open_file (a_file_name) ;
        source_editor = get_source_editor_from_path (a_file_name) ;
    }
    THROW_IF_FAIL (source_editor) ;
    source_editor->set_visual_breakpoint_at_line (a_linenum) ;
}

void
DBGPerspective::delete_visual_breakpoint (const UString &a_file_name, int a_linenum)
{
    SourceEditor *source_editor = get_source_editor_from_path (a_file_name) ;
    THROW_IF_FAIL (source_editor) ;
    source_editor->remove_visual_breakpoint_from_line (a_linenum) ;
}

void
DBGPerspective::delete_visual_breakpoint (int a_breakpoint_num)
{
    map<int, IDebugger::BreakPoint>::iterator iter =
        m_priv->breakpoints.find (a_breakpoint_num) ;
    if (iter == m_priv->breakpoints.end ()) {
        LOG_ERROR ("breakpoint " << (int) a_breakpoint_num << " not found") ;
        return ;
    }

    SourceEditor *source_editor =
        get_source_editor_from_path (iter->second.full_file_name ()) ;
    THROW_IF_FAIL (source_editor) ;
    source_editor->remove_visual_breakpoint_from_line (iter->second.line ()-1) ;
    m_priv->breakpoints.erase (iter);
}

bool
DBGPerspective::delete_breakpoint (const UString &a_file_name,
                                   int a_line_num)
{
    int breakpoint_number=0 ;
    if (!get_breakpoint_number (a_file_name,
                                a_line_num,
                                breakpoint_number)) {
        return false ;
    }
    if (breakpoint_number < 1) {return false;}

    return delete_breakpoint (breakpoint_number) ;
}

bool
DBGPerspective::is_breakpoint_set_at_line (const UString &a_file_path,
                                           int a_line_num)
{
    int break_num=0 ;
    if (get_breakpoint_number (a_file_path, a_line_num, break_num)) {
        return true ;
    }
    return false ;
}

void
DBGPerspective::toggle_breakpoint (const UString &a_file_path,
                                   int a_line_num)
{
    LOG_DD ("file_path:" << a_file_path
           << ", line_num: " << a_file_path) ;

    if (is_breakpoint_set_at_line (a_file_path, a_line_num)) {
        LOG_DD ("breakpoint set already, delete it!") ;
        delete_breakpoint (a_file_path, a_line_num) ;
    } else {
        LOG_DD ("breakpoint no set yet, set it!") ;
        set_breakpoint (a_file_path, a_line_num) ;
    }
}

void
DBGPerspective::toggle_breakpoint ()
{
    SourceEditor *source_editor = get_current_source_editor () ;
    THROW_IF_FAIL (source_editor) ;
    THROW_IF_FAIL (source_editor->get_path () != "") ;

    gint current_line =
        source_editor->source_view ().get_source_buffer ()->get_insert
                ()->get_iter ().get_line () + 1;
    toggle_breakpoint (source_editor->get_path (), current_line) ;
}

IDebuggerSafePtr&
DBGPerspective::debugger ()
{
    if (!m_priv->debugger) {
        THROW_IF_FAIL (m_priv->workbench) ;

        DynamicModule::Loader *loader = m_priv->workbench->get_module_loader () ;
        THROW_IF_FAIL (loader) ;
        DynamicModuleManager *module_manager =
                            loader->get_dynamic_module_manager () ;
        THROW_IF_FAIL (module_manager) ;

        m_priv->debugger =
            module_manager->load<IDebugger> ("gdbengine") ;
        m_priv->debugger->set_event_loop_context
                                    (Glib::MainContext::get_default ()) ;
    }
    THROW_IF_FAIL (m_priv->debugger) ;
    return m_priv->debugger ;
}

Gtk::TextView&
DBGPerspective::get_command_view ()
{
    THROW_IF_FAIL (m_priv && m_priv->command_view) ;
    return *m_priv->command_view ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_command_view_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;

    if (!m_priv->command_view_scrolled_win) {
        m_priv->command_view_scrolled_win = new Gtk::ScrolledWindow ;
        m_priv->command_view_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                       Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->command_view_scrolled_win) ;
    }
    return *m_priv->command_view_scrolled_win ;
}

Gtk::TextView&
DBGPerspective::get_target_output_view ()
{
    THROW_IF_FAIL (m_priv && m_priv->target_output_view) ;
    return *m_priv->target_output_view ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_target_output_view_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->target_output_view_scrolled_win) {
        m_priv->target_output_view_scrolled_win =  new Gtk::ScrolledWindow ;
        m_priv->target_output_view_scrolled_win->set_policy
                                                    (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->target_output_view_scrolled_win) ;
    }
    return *m_priv->target_output_view_scrolled_win ;
}

Gtk::TextView&
DBGPerspective::get_log_view ()
{
    THROW_IF_FAIL (m_priv && m_priv->log_view) ;
    return *m_priv->log_view ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_log_view_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->log_view_scrolled_win) {
        m_priv->log_view_scrolled_win = new Gtk::ScrolledWindow ;
        m_priv->log_view_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->log_view_scrolled_win) ;
    }
    return *m_priv->log_view_scrolled_win ;
}

CallStack&
DBGPerspective::get_call_stack ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->call_stack) {
        m_priv->call_stack = new CallStack (debugger ()) ;
        THROW_IF_FAIL (m_priv) ;
    }
    return *m_priv->call_stack ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_call_stack_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->call_stack_scrolled_win) {
        m_priv->call_stack_scrolled_win = new Gtk::ScrolledWindow () ;
        m_priv->call_stack_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
        THROW_IF_FAIL (m_priv->call_stack_scrolled_win) ;
    }
    return *m_priv->call_stack_scrolled_win ;
}

VarsEditor&
DBGPerspective::get_variables_editor ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->variables_editor) {
        m_priv->variables_editor = new VarsEditor (debugger ()) ;
    }
    THROW_IF_FAIL (m_priv->variables_editor) ;
    return *m_priv->variables_editor ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_variables_editor_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->variables_editor_scrolled_win) {
        m_priv->variables_editor_scrolled_win = new Gtk::ScrolledWindow ;
        m_priv->variables_editor_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                     Gtk::POLICY_AUTOMATIC) ;
    }
    THROW_IF_FAIL (m_priv->variables_editor_scrolled_win) ;
    return *m_priv->variables_editor_scrolled_win ;
}


Terminal&
DBGPerspective::get_terminal ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->terminal) {
        m_priv->terminal = new Terminal ;
    }
    THROW_IF_FAIL (m_priv->terminal) ;
    return *m_priv->terminal ;
}

Gtk::ScrolledWindow&
DBGPerspective::get_terminal_scrolled_win ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->terminal_scrolled_win) {
        m_priv->terminal_scrolled_win = new Gtk::ScrolledWindow ;
        THROW_IF_FAIL (m_priv->terminal_scrolled_win) ;
        m_priv->terminal_scrolled_win->set_policy (Gtk::POLICY_AUTOMATIC,
                                                   Gtk::POLICY_AUTOMATIC) ;
    }
    THROW_IF_FAIL (m_priv->terminal_scrolled_win) ;
    return *m_priv->terminal_scrolled_win ;
}

void
DBGPerspective::set_show_command_view (bool a_show)
{
    if (a_show) {
        if (!get_command_view_scrolled_win ().get_parent ()
            && m_priv->command_view_is_visible == false) {
            get_command_view_scrolled_win ().show_all () ;
            int pagenum =
            m_priv->statuses_notebook->insert_page
                            (get_command_view_scrolled_win (),
                             _("Commands"),
                             COMMAND_VIEW_INDEX) ;
            m_priv->statuses_notebook->set_current_page (pagenum) ;
            m_priv->command_view_is_visible = true ;
        }
    } else {
        if (get_command_view_scrolled_win ().get_parent ()
            && m_priv->command_view_is_visible) {
            m_priv->statuses_notebook->remove_page
                                        (get_command_view_scrolled_win ());
            m_priv->command_view_is_visible = false;
        }
    }
    show_command_view_signal ().emit (a_show) ;
}

void
DBGPerspective::set_show_target_output_view (bool a_show)
{
    if (a_show) {
        if (!get_target_output_view_scrolled_win ().get_parent ()
            && m_priv->target_output_view_is_visible == false) {
            get_target_output_view_scrolled_win ().show_all () ;
            int page_num =
                m_priv->statuses_notebook->insert_page
                    (get_target_output_view_scrolled_win (),
                     _("Output"),
                     TARGET_OUTPUT_VIEW_INDEX) ;
            m_priv->target_output_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_target_output_view_scrolled_win ().get_parent ()
            && m_priv->target_output_view_is_visible) {
            m_priv->statuses_notebook->remove_page
                                    (get_target_output_view_scrolled_win ());
            m_priv->target_output_view_is_visible = false;
        }
        m_priv->target_output_view_is_visible = false;
    }
    show_target_output_view_signal ().emit (a_show) ;
}

void
DBGPerspective::set_show_log_view (bool a_show)
{
    if (a_show) {
        if (!get_log_view_scrolled_win ().get_parent ()
            && m_priv->log_view_is_visible == false) {
            get_log_view_scrolled_win ().show_all () ;
            int page_num =
                m_priv->statuses_notebook->insert_page
                    (get_log_view_scrolled_win (), _("Logs"), ERROR_VIEW_INDEX) ;
            m_priv->log_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_log_view_scrolled_win ().get_parent ()
            && m_priv->log_view_is_visible) {
            LOG_DD ("removing error view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_log_view_scrolled_win ());
        }
        m_priv->log_view_is_visible = false;
    }
    show_log_view_signal ().emit (a_show) ;
}

void
DBGPerspective::set_show_call_stack_view (bool a_show)
{
    if (a_show) {
        if (!get_call_stack_scrolled_win ().get_parent ()
            && m_priv->call_stack_view_is_visible == false) {
            get_call_stack_scrolled_win ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_call_stack_scrolled_win (),
                                             _("Call stack"),
                                             CALL_STACK_VIEW_INDEX) ;
            m_priv->call_stack_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page
                                                (page_num);
        }
    } else {
        if (get_call_stack_scrolled_win ().get_parent ()
            && m_priv->call_stack_view_is_visible) {
            LOG_DD ("removing error view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_call_stack_scrolled_win ());
            m_priv->call_stack_view_is_visible = false;
        }
        m_priv->call_stack_view_is_visible = false;
    }
}

void
DBGPerspective::set_show_variables_editor_view (bool a_show)
{
    if (a_show) {
        if (!get_variables_editor_scrolled_win ().get_parent ()
            && m_priv->variables_editor_view_is_visible == false) {
            get_variables_editor_scrolled_win ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_variables_editor_scrolled_win (),
                                             _("Variables"),
                                             VARIABLES_VIEW_INDEX) ;
            m_priv->variables_editor_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page
                                                (page_num);
        }
    } else {
        if (get_variables_editor_scrolled_win ().get_parent ()
            && m_priv->variables_editor_view_is_visible) {
            LOG_DD ("removing error view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_variables_editor_scrolled_win ());
            m_priv->variables_editor_view_is_visible = false;
        }
        m_priv->variables_editor_view_is_visible = false;
    }
}

void
DBGPerspective::set_show_terminal_view (bool a_show)
{
    if (a_show) {
        if (!get_terminal_scrolled_win ().get_parent ()
            && m_priv->terminal_view_is_visible == false) {
            get_terminal_scrolled_win ().show_all () ;
            int page_num = m_priv->statuses_notebook->insert_page
                                            (get_terminal_scrolled_win (),
                                             _("Target terminal"),
                                             TERMINAL_VIEW_INDEX) ;
            m_priv->terminal_view_is_visible = true ;
            m_priv->statuses_notebook->set_current_page (page_num);
        }
    } else {
        if (get_terminal_scrolled_win ().get_parent ()
            && m_priv->terminal_view_is_visible) {
            LOG_DD ("removing error view") ;
            m_priv->statuses_notebook->remove_page
                                        (get_terminal_scrolled_win ());
            m_priv->terminal_view_is_visible = false;
        }
        m_priv->terminal_view_is_visible = false;
    }
}

struct ScrollTextViewToEndClosure {
    Gtk::TextView* text_view ;

    ScrollTextViewToEndClosure (Gtk::TextView *a_view=NULL) :
        text_view (a_view)
    {
    }

    bool do_exec ()
    {
        if (!text_view) {return false;}
        if (!text_view->get_buffer ()) {return false;}

        Gtk::TextIter end_iter = text_view->get_buffer ()->end () ;
        text_view->scroll_to (end_iter) ;
        return false ;
    }
};//end struct ScrollTextViewToEndClosure

void
DBGPerspective::add_text_to_command_view (const UString &a_text,
                                          bool a_no_repeat)
{
    if (a_no_repeat) {
        if (a_text == m_priv->last_command_text)
            return ;
    }
    THROW_IF_FAIL (m_priv && m_priv->command_view) ;
    m_priv->command_view->get_buffer ()->insert
        (get_command_view ().get_buffer ()->end (), a_text ) ;
    static ScrollTextViewToEndClosure s_scroll_to_end_closure ;
    s_scroll_to_end_closure.text_view = m_priv->command_view.get () ;
    Glib::signal_idle ().connect (sigc::mem_fun
            (s_scroll_to_end_closure, &ScrollTextViewToEndClosure::do_exec)) ;
    m_priv->last_command_text = a_text ;
}

void
DBGPerspective::add_text_to_target_output_view (const UString &a_text)
{
    THROW_IF_FAIL (m_priv && m_priv->target_output_view) ;
    m_priv->target_output_view->get_buffer ()->insert
        (get_target_output_view ().get_buffer ()->end (),
         a_text) ;
    static ScrollTextViewToEndClosure s_scroll_to_end_closure ;
    s_scroll_to_end_closure.text_view = m_priv->target_output_view.get () ;
    Glib::signal_idle ().connect (sigc::mem_fun
            (s_scroll_to_end_closure, &ScrollTextViewToEndClosure::do_exec)) ;
}

void
DBGPerspective::add_text_to_log_view (const UString &a_text)
{
    THROW_IF_FAIL (m_priv && m_priv->log_view) ;
    m_priv->log_view->get_buffer ()->insert
        (get_log_view ().get_buffer ()->end (), a_text) ;
    static ScrollTextViewToEndClosure s_scroll_to_end_closure ;
    s_scroll_to_end_closure.text_view = m_priv->log_view.get () ;
    Glib::signal_idle ().connect (sigc::mem_fun
            (s_scroll_to_end_closure, &ScrollTextViewToEndClosure::do_exec)) ;
}

sigc::signal<void, bool>&
DBGPerspective::activated_signal ()
{
    CHECK_P_INIT ;
    return m_priv->activated_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::attached_to_target_signal ()
{
    return m_priv->attached_to_target_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::debugger_ready_signal ()
{
    return m_priv->debugger_ready_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::show_command_view_signal ()
{
    return m_priv->show_command_view_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::show_target_output_view_signal ()
{
    return m_priv->show_target_output_view_signal ;
}

sigc::signal<void, bool>&
DBGPerspective::show_log_view_signal ()
{
    return m_priv->show_log_view_signal ;
}

}//end namespace nemiver

//the dynmod initial factory.
extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    gtksourceview::init () ;
    *a_new_instance = new nemiver::DBGPerspective () ;
    return (*a_new_instance != 0) ;
}

}//end extern C
