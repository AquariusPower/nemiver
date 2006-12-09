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
#ifndef __NEMIVER_I_DEBUGGER_H__
#define __NEMIVER_I_DEBUGGER_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include "nmv-api-macros.h"
#include "nmv-ustring.h"
#include "nmv-dynamic-module.h"
#include "nmv-safe-ptr-utils.h"

namespace nemiver {

using nemiver::common::SafePtr ;
using nemiver::common::DynamicModule ;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString ;
using nemiver::common::Object ;
using std::vector ;
using std::string ;
using std::map ;
using std::list;

class IDebugger ;
typedef SafePtr<IDebugger, ObjectRef, ObjectUnref> IDebuggerSafePtr ;

/// \brief a debugger engine.
///
///It should abstract the real debugger used underneath, but
///it is modeled after the interfaces exposed by gdb.
///Please, read GDB/MI interface documentation for more.
class NEMIVER_API IDebugger : public DynamicModule {

    IDebugger (const IDebugger&) ;
    IDebugger& operator= (const IDebugger&) ;

protected:
    IDebugger () {};

public:


    /// \brief a breakpoint descriptor
    class BreakPoint {
        int m_number ;
        bool m_enabled ;
        UString m_address ;
        UString m_function ;
        UString m_file_name ;
        UString m_file_full_name ;
        int m_line ;

    public:

        BreakPoint () {clear ();}

        /// \name accessors

        /// @{
        int number () const {return m_number ;}
        void number (int a_in) {m_number = a_in;}

        bool enabled () const {return m_enabled;}
        void enabled (bool a_in) {m_enabled = a_in;}

        const UString& address () const {return m_address;}
        void address (const UString &a_in) {m_address = a_in;}

        const UString& function () const {return m_function;}
        void function (const UString &a_in) {m_function = a_in;}

        const UString& file_name () const {return m_file_name;}
        void file_name (const UString &a_in) {m_file_name = a_in;}

        const UString& file_full_name () const {return m_file_full_name;}
        void file_full_name (const UString &a_in) {m_file_full_name = a_in;}

        int line () const {return m_line;}
        void line (int a_in) {m_line = a_in;}

        bool is_pending ()
        {
            if (m_address == "<PENDING>") {
                return true ;
            }
            return false ;
        }
        /// @}

        /// \brief clear this instance of breakpoint
        void clear ()
        {
            m_number = 0 ;
            m_enabled = false ;
            m_address = "" ;
            m_function = "" ;
            m_file_name = "" ;
            m_file_full_name = "" ;
            m_line = 0 ;
        }
    };//end class BreakPoint

    /// \brief a function frame as seen by the debugger.
    class Frame {
        UString m_address ;
        UString m_function_name ;
        map<UString, UString> m_args ;
        int m_level ;
        //present if the target has debugging info
        UString m_file_name ;
        //present if the target has sufficient debugging info
        UString m_file_full_name ;
        int m_line ;
        //present if the target doesn't have debugging info
        UString m_library ;
    public:

        Frame () {clear ();}

        /// \name accessors

        /// @{
        const UString& address () const {return m_address;}
        void address (const UString &a_in) {m_address = a_in;}

        const UString& function_name () const {return m_function_name ;}
        void function_name (const UString &a_in) {m_function_name = a_in;}

        const map<UString, UString>& args () const {return m_args;}
        map<UString, UString>& args () {return m_args;}

        int level () const {return m_level;}
        void level (int a_level) {m_level = a_level;}

        const UString& file_name () const {return m_file_name;}
        void file_name (const UString &a_in) {m_file_name = a_in;}

        const UString& file_full_name () const {return m_file_full_name;}
        void file_full_name (const UString &a_in) {m_file_full_name = a_in;}

        int line () const {return m_line;}
        void line (int a_in) {m_line = a_in;}

        const UString& library () const {return m_library;}
        void library (const UString &a_library) {m_library = a_library;}

        /// @}

        /// \brief clears the current instance
        void clear ()
        {
            m_address = "" ;
            m_function_name = "" ;
            m_args.clear () ;
            m_level = 0 ;
            m_file_name = "" ;
            m_file_full_name = "" ;
            m_line = 0 ;
            m_library.clear () ;
            m_args.clear () ;
        }
    };//end class Frame

    class Variable ;
    typedef SafePtr<Variable, ObjectRef, ObjectUnref> VariableSafePtr ;
    class Variable : public Object {
        list<VariableSafePtr> m_members;
        UString m_name ;
        UString m_value ;
        UString m_type ;
        Variable *m_parent ;

    public:

        Variable (const UString &a_name,
                  const UString &a_value,
                  const UString &a_type) :
            m_name (a_name),
            m_value (a_value),
            m_type (a_type),
            m_parent (0)

        {}

        Variable (const UString &a_name) :
            m_name (a_name),
            m_parent (0)
        {}

        Variable () :
            m_parent (0)
        {}

        const list<VariableSafePtr>& members () const {return m_members;}

        void append (const VariableSafePtr &a_var)
        {
            if (!a_var) {return;}
            m_members.push_back (a_var);
            a_var->parent (this) ;
        }

        const UString& name () const {return m_name;}
        void name (const UString &a_name) {m_name = a_name;}

        const UString& value () const {return m_value;}
        void value (const UString &a_value) {m_value = a_value;}

        const UString& type () const {return m_type;}
        void type (const UString &a_type) {m_type = a_type;}

        Variable* parent () const {return m_parent ;}
        void parent (Variable *a_parent)
        {
            m_parent = a_parent ;
        }

        void to_string (UString &a_str,
                        bool a_show_var_name = false,
                        const UString &a_indent_str="") const
        {
            if (a_show_var_name) {
                if (name () != "") {
                    a_str += a_indent_str + name () ;
                }
            }
            if (value () != "") {
                if (a_show_var_name) {
                    a_str += "=" ;
                }
                a_str += value () ;
            }
            if (members ().empty ()) {
                return ;
            }
            UString indent_str = a_indent_str + "  " ;
            a_str += "\n" + a_indent_str + "{";
            list<VariableSafePtr>::const_iterator it ;
            for (it = members ().begin () ; it != members ().end () ; ++it) {
                if (!(*it)) {continue;}
                a_str += "\n" ;
                (*it)->to_string (a_str, true, indent_str) ;
            }
            a_str += "\n" + a_indent_str + "}";
            a_str.chomp () ;
        }

        void build_qname (UString &a_qname) const
        {
            UString qname ;
            if (parent () == 0) {
                a_qname = name ();
                if (a_qname[0] == '*') {
                    a_qname.erase (0, 1) ;
                }
            } else {
                parent ()->build_qname (qname) ;
                qname.chomp () ;
                if (parent ()->name ()[0] == '*') {
                    qname += "->" + name () ;
                } else {
                    qname += "." + name () ;
                }
                a_qname = qname ;
            }
        }
    };//end class Variable

    enum State {
        READY = 0,
        RUNNING,
        PROGRAM_EXITED
    };//enum State


    virtual ~IDebugger () {}

    /// \name events you can connect to.

    /// @{

    virtual sigc::signal<void>& engine_died_signal () const = 0 ;

    virtual sigc::signal<void>& program_finished_signal () const = 0;

    virtual sigc::signal<void, const UString&>&
                                     console_message_signal () const = 0 ;

    virtual sigc::signal<void, const UString&>&
                                 target_output_message_signal () const = 0 ;

    virtual sigc::signal<void, const UString&>& log_message_signal () const = 0 ;

    virtual sigc::signal<void,
                         const UString&/*command name*/,
                         const UString&/*command cookie*/>&
                                                 command_done_signal () const = 0;

    virtual sigc::signal<void, const IDebugger::BreakPoint&, int>&
                                         breakpoint_deleted_signal () const  = 0;

    virtual sigc::signal<void, const map<int, IDebugger::BreakPoint>&>&
                                             breakpoints_set_signal () const = 0;

    virtual sigc::signal<void,
                         const UString&/*reason*/,
                         bool /*has frame*/,
                         const IDebugger::Frame&/*the frame*/,
                         int /*thread id*/>& stopped_signal () const = 0;

    virtual sigc::signal<void, const list<int> >&
                                        threads_listed_signal () const = 0;

    virtual sigc::signal<void,
                         int/*thread id*/,
                         const IDebugger::Frame&/*frame in thread*/> &
                                            thread_selected_signal () const = 0 ;

    virtual sigc::signal<void, const vector<IDebugger::Frame>& >&
                                                frames_listed_signal () const=0;

    virtual sigc::signal<void,
                         const map<int, list<IDebugger::VariableSafePtr> >&>&
                                        frames_params_listed_signal () const=0;

    /// called when a core file is loaded.
    /// it signals the current frame, i.e the frame in which
    /// the core got dumped.
    virtual sigc::signal<void, const IDebugger::Frame&>&
                                            current_frame_signal () const = 0 ;


    virtual sigc::signal<void, const list<VariableSafePtr>& >&
                            local_variables_listed_signal () const = 0;

    virtual sigc::signal<void,
                         const UString&/*variable name*/,
                         const VariableSafePtr&/*variable*/>&
                                            variable_value_signal () const = 0 ;

    virtual sigc::signal<void,
                         const UString&/*variable name*/,
                         const VariableSafePtr&/*variable*/>&
                                    pointed_variable_value_signal () const = 0 ;

    virtual sigc::signal<void,
                         const UString&/*variable name*/,
                         const UString&/*type*/>&
                                        variable_type_signal () const = 0 ;

    virtual sigc::signal<void,
                         int/*pid*/,
                         const UString&/*target path*/>&
                                            got_target_info_signal () const = 0 ;

    virtual sigc::signal<void>& running_signal () const = 0;

    virtual sigc::signal<void,
                         const UString&/*signal name*/,
                         const UString&/*signal description*/>&
                                            signal_received_signal () const = 0;

    virtual sigc::signal<void, const UString&/*error message*/>&
                                                    error_signal () const = 0;

    virtual sigc::signal<void, IDebugger::State>& state_changed_signal () const=0;

    /// @}

    virtual map<UString, UString>& properties () = 0;

    virtual void set_event_loop_context
                        (const Glib::RefPtr<Glib::MainContext> &) = 0 ;

    virtual void run_loop_iterations (int a_nb_iters) = 0;

    /*
    virtual void execute_command (const Command &a_command) = 0;

    virtual bool queue_command (const Command &a_command) = 0;
    */

    virtual bool busy () const = 0;

    virtual void load_program
                (const vector<UString> &a_argv,
                 const UString &working_dir,
                 const vector<UString> &a_source_search_dirs,
                 const UString &a_tty_path="") = 0;

    virtual void load_core_file (const UString &a_prog_file,
                                 const UString &a_core_file) = 0;

    virtual bool attach_to_program (unsigned int a_pid,
                                    const UString &a_tty_path="") = 0;

    virtual void add_env_variables (const map<UString, UString> &a_vars) = 0;

    virtual map<UString, UString>& get_env_variables () = 0 ;

    virtual const UString& get_target_path () = 0 ;

    virtual void do_continue (const UString &a_cookie="") = 0;

    virtual void run (const UString &a_cookie="") = 0 ;

    virtual void get_target_info (const UString &a_cookie="") = 0 ;

    virtual bool stop () = 0 ;

    virtual void step_in (const UString &a_cookie="") = 0;

    virtual void step_over (const UString &a_cookie="") = 0;

    virtual void step_out (const UString &a_cookie="") = 0;

    virtual void continue_to_position (const UString &a_path,
                                       gint a_line_num,
                                       const UString &a_cookie="") = 0 ;
    virtual void set_breakpoint (const UString &a_path,
                                 gint a_line_num,
                                 const UString &a_cookie="") = 0 ;
    virtual void set_breakpoint (const UString &a_func_name,
                                 const UString &a_cookie="") = 0 ;

    virtual void list_breakpoints (const UString &a_cookie="") = 0 ;

    virtual const map<int, BreakPoint>& get_cached_breakpoints () = 0 ;

    virtual void enable_breakpoint (const UString &a_path,
                                    gint a_line_num,
                                    const UString &a_cookie="") = 0;

    virtual void disable_breakpoint (const UString &a_path,
                                     gint a_line_num,
                                     const UString &a_cookie="") = 0;

    virtual void delete_breakpoint (const UString &a_path,
                                    gint a_line_num,
                                    const UString &a_cookie="") = 0;

    virtual void delete_breakpoint (gint a_break_num,
                                    const UString &a_cookie="") = 0;

    virtual void list_threads (const UString &a_cookie="") = 0 ;

    virtual void select_thread (unsigned int a_thread_id,
                                const UString &a_cookie="") = 0;

    virtual void select_frame (int a_frame_id,
                               const UString &a_cookie="") = 0;

    virtual void list_frames (const UString &a_cookie="") = 0;

    virtual void list_frames_arguments (int a_low_frame=-1,
                                        int a_high_frame=-1,
                                        const UString &a_cookie="") = 0;

    virtual void list_local_variables (const UString &a_cookie="")  = 0;

    virtual void evaluate_expression (const UString &a_expr,
                                      const UString &a_cookie="")  = 0;

    virtual void print_variable_value (const UString &a_var_name,
                                       const UString &a_cookie="")  = 0;

    virtual void print_pointed_variable_value (const UString &a_var_name,
                                               const UString &a_cookie="") = 0;

    virtual void print_variable_type (const UString &a_var_name,
                                      const UString &a_cookie="") = 0;

};//end IDebugger

}//end namespace nemiver

#endif //__NEMIVER_I_DEBUGGER_H__

