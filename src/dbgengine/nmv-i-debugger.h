// -*- c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-'
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
#ifndef __NEMIVER_I_DEBUGGER_H__
#define __NEMIVER_I_DEBUGGER_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <list>
#include "common/nmv-api-macros.h"
#include "common/nmv-ustring.h"
#include "common/nmv-dynamic-module.h"
#include "common/nmv-safe-ptr-utils.h"
#include "nmv-i-conf-mgr.h"

NEMIVER_BEGIN_NAMESPACE (nemiver)

using nemiver::common::SafePtr;
using nemiver::common::DynamicModule;
using nemiver::common::DynamicModuleSafePtr;
using nemiver::common::DynModIface;
using nemiver::common::ObjectRef;
using nemiver::common::ObjectUnref;
using nemiver::common::UString;
using nemiver::common::Object;
using std::vector;
using std::string;
using std::map;
using std::list;

class ILangTrait;
class IDebugger;
typedef SafePtr<IDebugger, ObjectRef, ObjectUnref> IDebuggerSafePtr;

/// \brief a debugger engine.
///
///It should abstract the real debugger used underneath, but
///it is modeled after the interfaces exposed by gdb.
///Please, read GDB/MI interface documentation for more.
class NEMIVER_API IDebugger : public DynModIface {

    IDebugger (const IDebugger&);
    IDebugger& operator= (const IDebugger&);

protected:

    IDebugger (DynamicModule *a_dynmod) : DynModIface (a_dynmod)
    {
    }

public:

    typedef unsigned int register_id_t;

    /// \brief a breakpoint descriptor
    class Breakpoint {
    public:

        enum Type {
            UNDEFINED_TYPE = 0,
            STANDARD_BREAKPOINT_TYPE,
            WATCHPOINT_TYPE
        };

    private:
        int m_number;
        bool m_enabled;
        UString m_address;
        UString m_function;
        UString m_expression;
        UString m_file_name;
        UString m_file_full_name;
        UString m_condition;
        Type m_type;
        int m_line;
        int m_nb_times_hit;
        int m_ignore_count;
        bool m_is_read_watchpoint;
        bool m_is_write_watchpoint;

    public:
        Breakpoint () {clear ();}

        /// \name accessors

        /// @{
        int number () const {return m_number;}
        void number (int a_in) {m_number = a_in;}

        bool enabled () const {return m_enabled;}
        void enabled (bool a_in) {m_enabled = a_in;}

        const UString& address () const {return m_address;}
        void address (const UString &a_in) {m_address = a_in;}

        const UString& function () const {return m_function;}
        void function (const UString &a_in) {m_function = a_in;}

        const UString& expression () const {return m_expression;}
        void expression (const UString &a_expr) {m_expression = a_expr;}

        const UString& file_name () const {return m_file_name;}
        void file_name (const UString &a_in) {m_file_name = a_in;}

        const UString& file_full_name () const {return m_file_full_name;}
        void file_full_name (const UString &a_in) {m_file_full_name = a_in;}

        int line () const {return m_line;}
        void line (int a_in) {m_line = a_in;}

        const UString& condition () const {return m_condition;}
        void condition (const UString &a_cond) {m_condition = a_cond;}

        bool has_condition () const {return !m_condition.empty ();}

        int nb_times_hit () const {return m_nb_times_hit;}
        void nb_times_hit (int a_nb) {m_nb_times_hit = a_nb;}

        int ignore_count () const {return m_ignore_count;}
        void ignore_count (int a) {m_ignore_count = a;}

        bool is_read_watchpoint () const {return m_is_read_watchpoint;}
        void is_read_watchpoint (bool f) {m_is_read_watchpoint = f;}

        bool is_write_watchpoint () const {return m_is_write_watchpoint;}
        void is_write_watchpoint (bool f) {m_is_write_watchpoint = f;}

        bool is_pending ()
        {
            if (m_address == "<PENDING>") {
                return true;
            }
            return false;
        }

        Type type () const {return m_type;}
        void type (Type a_type) {m_type = a_type;}

        /// @}

        /// \brief clear this instance of breakpoint
        void clear ()
        {
            m_type = STANDARD_BREAKPOINT_TYPE;
            m_number = 0;
            m_enabled = false;
            m_address.clear ();
            m_function.clear ();
            m_file_name.clear ();
            m_file_full_name.clear ();
            m_line = 0;
            m_condition.clear ();
            m_nb_times_hit = 0;
            m_ignore_count = 0;
            m_is_read_watchpoint = false;
            m_is_write_watchpoint = false;
        }
    };//end class Breakpoint

    /// \brief an entry of a choice list
    /// to choose between a list of overloaded
    /// functions. This is used for instance
    /// to propose a choice between several
    /// overloaded functions when the user asked
    /// to break into a function by name and when
    /// that name has several overloads.
    class OverloadsChoiceEntry {
    public:
        enum Kind {
            CANCEL=0,
            ALL,
            LOCATION
        };

    private:
        Kind m_kind;
        int m_index;
        UString m_function_name;
        UString m_file_name;
        int m_line_number;

        void init (Kind a_kind, int a_index,
                   const UString &a_function_name,
                   const UString &a_file_name,
                   int a_line_number)
        {
            kind (a_kind);
            index (a_index);
            function_name (a_function_name);
            file_name (a_file_name);
            line_number (a_line_number);
        }

    public:
        OverloadsChoiceEntry (Kind a_kind,
                              int a_index,
                              UString &a_function_name,
                              UString a_file_name,
                              int a_line_number)
        {
            init (a_kind, a_index, a_function_name,
                  a_file_name, a_line_number);
        }

        OverloadsChoiceEntry ()
        {
            init (CANCEL, 0, "", "", 0);
        }

        Kind kind () const {return m_kind;}
        void kind (Kind a_kind) {m_kind = a_kind;}

        int index () const {return m_index;}
        void index (int a_index) {m_index = a_index;}

        const UString& function_name () const {return m_function_name;}
        void function_name (const UString& a_in) {m_function_name = a_in;}

        const UString& file_name () const {return m_file_name;}
        void file_name (const UString &a_in) {m_file_name = a_in;}

        int line_number () const {return m_line_number;}
        void line_number (int a_in) {m_line_number = a_in;}
    };//end class OverloadsChoiceEntry

    /// \brief a function frame as seen by the debugger.
    class Frame {
        UString m_address;
        UString m_function_name;
        map<UString, UString> m_args;
        int m_level;
        //present if the target has debugging info
        UString m_file_name;
        //present if the target has sufficient debugging info
        UString m_file_full_name;
        int m_line;
        //present if the target doesn't have debugging info
        UString m_library;
    public:

        Frame () :
            m_level (0),
            m_line (0)
        {}

        /// \name accessors

        /// @{
        const UString& address () const {return m_address;}
        void address (const UString &a_in) {m_address = a_in;}

        const UString& function_name () const {return m_function_name;}
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
            m_address = "";
            m_function_name = "";
            m_args.clear ();
            m_level = 0;
            m_file_name = "";
            m_file_full_name = "";
            m_line = 0;
            m_library.clear ();
            m_args.clear ();
        }
    };//end class Frame

    class Variable;
    typedef SafePtr<Variable, ObjectRef, ObjectUnref> VariableSafePtr;
    typedef list<VariableSafePtr> VariableList;
    class Variable : public Object {
        // non copyable.
        Variable (const Variable &);
        Variable& operator= (const Variable &);

        bool operator == (Variable &a_other) const;
        bool operator == (Variable &a_other);

        VariableList m_members;
        // If this variable was created with a backend counterpart
        // (e.g: backend side variable objects in GDB), then this
        // is the name of the backend side counterpart of this variable.
        UString m_internal_name;
        UString m_name;
        UString m_name_caption;
        UString m_value;
        UString m_type;
        Variable *m_parent;
        //if this variable is a pointer,
        //it can be dereferenced. The variable
        //it points to is stored in m_dereferenced
        VariableSafePtr m_dereferenced;
        unsigned int m_num_expected_children;
        // The expression with which this variable
        // Can be referenced in the debugger.
        // If empty, it can be set by calling
        // IDebugger::query_variable_path_expr()
        UString m_path_expression;

    public:
        Variable (const UString &a_internal_name,
                  const UString &a_name,
                  const UString &a_value,
                  const UString &a_type) :
            m_internal_name (a_internal_name),
            m_name (a_name),
            m_value (a_value),
            m_type (a_type),
            m_parent (0),
            m_num_expected_children (0)

        {
        }

        Variable (const UString &a_name,
                  const UString &a_value,
                  const UString &a_type) :
            m_name (a_name),
            m_value (a_value),
            m_type (a_type),
            m_parent (0),
            m_num_expected_children (0)

        {
        }

        Variable (const UString &a_name) :
            m_name (a_name),
            m_parent (0),
            m_num_expected_children (0)
        {}

        Variable () :
            m_parent (0),
            m_num_expected_children (0)
        {}

        const VariableList& members () const {return m_members;}

        /// Returns the Nth member variable, if any.
        /// Please note that this function has O(n) complexity. So use it with
        /// care.
        /// \param a_index, the index of the member variable to return,
        ///        starting from zero.
        /// \param a_var the output parameter. Is set if and only if the
        ///         the function returned true.
        /// \return true if there was a variable at the specified index,
        ///  false otherwise.
        bool get_member_at (int a_index,
                            VariableSafePtr &a_var) const
        {
            int i = 0;
            for (VariableList::const_iterator it = members ().begin ();
                 it != m_members.end ();
                 ++it, ++i) {
                if (i == a_index) {
                    a_var = *it;
                    return true;
                }
            }
            return false;
        }

        /// \return the sibbling index of the variable node.
        /// If first sibbling at a given level has index 0.
        int sibling_index () const
        {
            if (!parent ())
                return 0;
            VariableList::const_iterator it;
            int i = 0;
            for (it = parent ()->members ().begin ();
                 it != parent ()->members ().end ();
                 ++it, ++i) {
                if (*it == this)
                    return i;
            }
            THROW ("fatal: should not be reached");
        }

        /// Tests value equality between two variables.
        /// Two variables are considered equal by value if their
        /// respective memebers have the same values and same type.
        /// \param a_other the other variable to test against
        /// \return true if a_other equals the current instance
        ///         by value.
        bool equals_by_value (const Variable &a_other) const
        {
            if (name () != a_other.name ()
                || type () != a_other.type ())
                return false;
            if (members ().empty () != a_other.members ().empty ())
                return false;
            VariableList::const_iterator it0, it1;
            for (it0 = members ().begin (), it1 = a_other.members ().begin ();
                 it0 != members ().end ();
                 ++it0, ++it1) {
                if (it1 == a_other.members ().end ())
                    return false;
                if (!(*it0)->equals_by_value (**it1))
                    return false;
            }
            if (it1 != a_other.members ().end ())
                return false;
            return true;
        }

        void append (const VariableSafePtr &a_var)
        {
            if (!a_var) {return;}
            m_members.push_back (a_var);
            a_var->parent (this);
        }

        /// If this variable was created with a backend counterpart
        /// (e.g. backend side variable objects in GDB), then this
        /// returns the name of the backend side counterpart of this variable.
        /// \return the name of the backend side counterpart of this
        /// variable. It can be an empty string if there is no backend side
        /// counterpart variable object created for this object.
        const UString& internal_name () const {return m_internal_name;}

        /// Set the name of the backend side counterpart object.
        /// (e.g. variable object in GDB).
        /// \param a_in the new name of backend side counterpart variable object.
        void internal_name (const UString &a_in) {m_internal_name = a_in;}

        const UString& name () const {return m_name;}
        void name (const UString &a_name)
        {
            m_name_caption = a_name;
            m_name = a_name;
        }

        const UString& name_caption () const {return m_name_caption;}
        void name_caption (const UString &a_n) {m_name_caption = a_n;}

        const UString& value () const {return m_value;}
        void value (const UString &a_value) {m_value = a_value;}

        const UString& type () const {return m_type;}
        void type (const UString &a_type) {m_type = a_type;}
        void type (const string &a_type) {m_type = a_type;}

        /// Return true if this instance of Variable has a parent variable,
        /// false otherwise.
        bool has_parent () const
        {
            return m_parent != 0;
        }

        /// A getter of the parent Variable of the current instance.
        const VariableSafePtr parent () const
        {
            VariableSafePtr parent (m_parent, true/*add a reference*/);
            return parent;
        }

        /// Parent variable setter.
        /// We don't increase the ref count of the
        /// parent variable to avoid memory leaks due to refcounting cyles.
        /// The parent already holds a reference on this instance of
        /// Variable so we really should not hold a reference on it as
        /// well.
        void parent (Variable *a_parent)
        {
            m_parent = a_parent;
        }

        /// \return the root parent of this variable.
        const VariableSafePtr root () const
        {
            if (!has_parent ()) {
                return VariableSafePtr (this, true /*increase refcount*/);
            }
            return parent ()->root ();
        }

        void to_string (UString &a_str,
                        bool a_show_var_name = false,
                        const UString &a_indent_str="") const
        {
            if (a_show_var_name) {
                if (name () != "") {
                    a_str += a_indent_str + name ();
                }

                if (!internal_name ().empty ()) {
                    a_str += "(" + internal_name () + ")";
                }
            }
            if (value () != "") {
                if (a_show_var_name) {
                    a_str += "=";
                }
                a_str += value ();
            }
            if (members ().empty ()) {
                return;
            }
            UString indent_str = a_indent_str + "  ";
            a_str += "\n" + a_indent_str + "{";
            VariableList::const_iterator it;
            for (it = members ().begin (); it != members ().end (); ++it) {
                if (!(*it)) {continue;}
                a_str += "\n";
                (*it)->to_string (a_str, true, indent_str);
            }
            a_str += "\n" + a_indent_str + "}";
            a_str.chomp ();
        }

        void build_qname (UString &a_qname) const
        {
            UString qname;
            if (!parent ()) {
                a_qname = name ();
                if (!a_qname.raw ().empty () && a_qname.raw ()[0] == '*') {
                    a_qname.erase (0, 1);
                }
            } else if (parent ()) {
                parent ()->build_qname (qname);
                qname.chomp ();
                if (parent () && parent ()->name ()[0] == '*') {
                    qname += "->" + name ();
                } else {
                    qname += "." + name ();
                }
                a_qname = qname;
            } else {
                THROW ("should not be reached");
            }
        }

        void build_qualified_internal_name (UString &a_qname) const
        {
            UString qname;
            if (!parent ()) {
                a_qname = internal_name ();
            } else if (parent ()) {
                parent ()->build_qname (qname);
                qname.chomp ();
                qname += "." + name ();
                a_qname = qname;
            } else {
                THROW ("should not be reached");
            }
        }

        void set_dereferenced (VariableSafePtr a_derefed)
        {
            m_dereferenced  = a_derefed;
        }

        VariableSafePtr get_dereferenced ()
        {
            return m_dereferenced;
        }

        bool is_dereferenced ()
        {
            if (m_dereferenced) {return true;}
            return false;
        }

        bool is_copyable (const Variable &a_other) const
        {
            if (a_other.type () != type ()) {
                //can't copy a variable of different type
                return false;
            }
            //both variables must have same members
            if (members ().size () != a_other.members ().size ()) {
                return false;
            }

            VariableList::const_iterator it1, it2;
            //first make sure our members have the same types as their members
            for (it1=members ().begin (), it2=a_other.members ().begin ();
                 it1 != members ().end ();
                 it1++, it2++) {
                if (!*it1 || !*it2)
                    return false;
                if (!(*it1)->is_copyable (**it2))
                    return false;
            }
            return true;
        }

        bool copy (const Variable &a_other)
        {
            if (!is_copyable (a_other))
                return false;
            m_name = a_other.m_name;
            m_name_caption = a_other.m_name_caption;
            m_value = a_other.m_value;
            VariableList::iterator it1;
            VariableList::const_iterator it2;
            for (it1=m_members.begin (), it2=a_other.m_members.begin ();
                 it1 != m_members.end ();
                 it1++, it2++) {
                (*it1)->copy (**it2);
            }
            return true;
        }

        void set (const Variable &a_other)
        {
            m_name = a_other.m_name;
            m_value = a_other.m_value;
            m_type = a_other.m_type;
            VariableList::const_iterator it;
            m_members.clear ();
            for (it = a_other.m_members.begin ();
                 it != a_other.m_members.end ();
                 ++it) {
                VariableSafePtr var;
                var.reset (new Variable ());
                var->set (**it);
                append (var);
            }
        }

        unsigned int num_expected_children () const
        {
            return m_num_expected_children;
        }

        void num_expected_children (unsigned int a_in)
        {
            m_num_expected_children = a_in;
        }

        bool has_expected_children () const
        {
            return m_num_expected_children != 0;
        }

        /// \return true if the current variable needs to be unfolded
        /// by a call to IDebugger::unfold_variable()
        bool needs_unfolding () const
        {
            return (has_expected_children () && members ().empty ());
        }

        /// Return the descendant of the current instance of Variable.
        /// \param a_internal_path the internal fully qualified path of the
        ///        descendant variable.
        const VariableSafePtr get_descendant
                                (const UString &a_internal_path) const
        {
            VariableSafePtr result;
            if (internal_name () == a_internal_path) {
                result.reset (this, true /*take refcount*/);
                return result;
            }
            for (VariableList::const_iterator it = m_members.begin ();
                 it != m_members.end ();
                 ++it) {
                if (*it && (*it)->internal_name () == a_internal_path) {
                    return *it;
                }
                result = (*it)->get_descendant (a_internal_path);
                if (result) {
                    return result;
                }
            }
            return result;
        }

        const UString& path_expression () const
        {
            return m_path_expression;
        }
        void path_expression (const UString &a_expr)
        {
            m_path_expression = a_expr;
        }
    };//end class Variable

    enum State {
        NOT_STARTED=0,
        READY,
        RUNNING,
        PROGRAM_EXITED
    };//enum State

    static UString state_to_string (State a_state)
    {
        UString str;
        switch (a_state) {
            case NOT_STARTED:
                str = "NOT_STARTED";
                break;
            case READY:
                str = "READY";
                break;
            case RUNNING:
                str = "RUNNING";
                break;
            case PROGRAM_EXITED:
                str = "PROGRAM_EXITED";
                break;
        }
        return str;
    }

    enum StopReason {
        UNDEFINED_REASON=0,
        BREAKPOINT_HIT,
        WATCHPOINT_TRIGGER,
        READ_WATCHPOINT_TRIGGER,
        ACCESS_WATCHPOINT_TRIGGER,
        FUNCTION_FINISHED,
        LOCATION_REACHED,
        WATCHPOINT_SCOPE,
        END_STEPPING_RANGE,
        EXITED_SIGNALLED,
        EXITED,
        EXITED_NORMALLY,
        SIGNAL_RECEIVED
    };//end enum StopReason

    /// Assembly instruction type
    /// It carries the address of the instruction,
    /// the function the instruction is from, offset of the instruction
    /// starting from the beginning of the function, and the instruction
    /// itself, represented by a string.
    class AsmInstr {
        string m_address;
        string m_func;
        string m_offset;
        string m_instr;

    public:
        explicit AsmInstr ()
        {
        }

        AsmInstr (string &a_address,
                  string &a_func,
                  string &a_offset,
                  string &a_instr):
            m_address (a_address),
            m_func (a_func),
            m_offset (a_offset),
            m_instr (a_instr)
        {
        }

        virtual ~AsmInstr ()
        {
        }

        const string& address () const {return m_address;}
        void address (string &a) {m_address = a;}

        const string& function () const {return m_func;}
        void function (const string &a_str) {m_func = a_str;}

        const string& offset () const {return m_offset;}
        void offset (string &a_o) {m_offset = a_o;}

        const string& instruction () const {return m_instr;}
        void instruction (const string &a_instr) {m_instr = a_instr;}
    };//end class AsmInstr

    class DisassembleInfo {
        // no need of copy constructor yet,
        // as we don't have any pointer member.
        UString m_function_name;
        UString m_file_name;
        std::string m_start_address;
        std::string m_end_address;

    public:
        DisassembleInfo ()
        {
        }
        ~DisassembleInfo ()
        {
        }

        const UString& function_name () const {return m_function_name;}
        void function_name (const UString &a_name) {m_function_name = a_name;}

        const UString& file_name () const {return m_file_name;}
        void file_name (const UString &a_name) {m_file_name = a_name;}

        const std::string& start_address () const {return m_start_address;}
        void start_address (const std::string &a) {m_start_address = a;}

        const std::string& end_address () const {return m_end_address;}
        void end_address (const std::string &a) {m_end_address = a;}
    };// end class DisassembleInfo

    virtual ~IDebugger () {}

    /// \name events you can connect to.

    /// @{

    virtual sigc::signal<void>& engine_died_signal () const = 0;

    virtual sigc::signal<void>& program_finished_signal () const = 0;

    virtual sigc::signal<void, const UString&>&
                                     console_message_signal () const = 0;

    virtual sigc::signal<void, const UString&>&
                                 target_output_message_signal () const = 0;

    virtual sigc::signal<void, const UString&>& log_message_signal () const=0;

    virtual sigc::signal<void,
                         const UString&/*command name*/,
                         const UString&/*command cookie*/>&
                                             command_done_signal () const=0;

    virtual sigc::signal<void>& connected_to_server_signal () const=0;

    virtual sigc::signal<void>& detached_from_target_signal () const=0;

    virtual sigc::signal<void,
                        const IDebugger::Breakpoint&,
                        int /*breakpoint command*/,
                        const UString & /*cookie*/>&
                                     breakpoint_deleted_signal () const=0;

    /// returns a list of breakpoints set. It is not all the breakpoints
    /// set. Some of the breakpoints in the list can have been
    /// already returned in a previous call. This is done so because
    /// IDebugger does not cache the list of breakpoints. This must
    /// be fixed at some point.
    virtual sigc::signal<void,
                         const map<int, IDebugger::Breakpoint>&,
                         const UString& /*cookie*/>&
                                         breakpoints_set_signal () const=0;

    virtual sigc::signal<void,
                         const vector<OverloadsChoiceEntry>&,
                         const UString& /*cookie*/>&
                                        got_overloads_choice_signal () const=0;

    virtual sigc::signal<void,
                         IDebugger::StopReason /*reason*/,
                         bool /*has frame*/,
                         const IDebugger::Frame&/*the frame*/,
                         int /*thread id*/,
                         int /*breakpoint number,
                               meaningfull only when
                               reason == IDebugger::BREAKPOINT_HIT*/,
                         const UString& /*cookie*/>& stopped_signal () const=0;

    virtual sigc::signal<void,
                         const list<int>/*thread ids*/,
                         const UString& /*cookie*/>&
                                        threads_listed_signal () const =0;

    virtual sigc::signal<void,
                         int/*thread id*/,
                         const IDebugger::Frame *const/*frame in thread*/,
                         const UString& /*cookie*/> &
                                             thread_selected_signal () const=0;

    virtual sigc::signal<void,
                        const vector<IDebugger::Frame>&,
                        const UString&>& frames_listed_signal () const=0;

    virtual sigc::signal<void,
                         //a frame number/argument list map
                         const map<int, list<IDebugger::VariableSafePtr> >&,
                         const UString& /*cookie*/>&
                                    frames_arguments_listed_signal () const=0;

    /// called when a core file is loaded.
    /// it signals the current frame, i.e the frame in which
    /// the core got dumped.
    virtual sigc::signal<void,
                        const IDebugger::Frame&,
                        const UString& /*cookie*/>&
                                            current_frame_signal () const = 0;


    virtual sigc::signal<void, const VariableList&, const UString& >&
                            local_variables_listed_signal () const = 0;

    virtual sigc::signal<void, const VariableList&, const UString& >&
                            global_variables_listed_signal () const = 0;

    /// Emitted as the result of the IDebugger::print_variable_value() call.
    virtual sigc::signal<void,
                         const UString&/*variable name*/,
                         const VariableSafePtr /*variable*/,
                         const UString& /*cookie*/>&
                                             variable_value_signal () const = 0;

    /// Emitted as the result of the IDebugger::get_variable_value() call.
    virtual sigc::signal<void,
                         const VariableSafePtr /*variable*/,
                         const UString& /*cookie*/>&
                                     variable_value_set_signal () const = 0;

    virtual sigc::signal<void,
                         const UString&/*variable name*/,
                         const VariableSafePtr /*variable*/,
                         const UString& /*cookie*/>&
                                    pointed_variable_value_signal () const = 0;

    /// Emitted as the result of the IDebugger::print_variable_type() call.
    virtual sigc::signal<void,
                         const UString&/*variable name*/,
                         const UString&/*type*/,
                         const UString&/*cookie*/>&
                                        variable_type_signal () const = 0;
    virtual sigc::signal<void,
                         const VariableSafePtr /*variable*/,
                         const UString&/*cookie*/>&
                                    variable_type_set_signal () const=0;

    /// Emitted as a the result of the IDebugger::dereference_variable is call.
    virtual sigc::signal<void,
                         const VariableSafePtr/*the variable we derefed*/,
                         const UString&/*cookie*/>
                                      variable_dereferenced_signal () const=0;

    virtual sigc::signal<void, const vector<UString>&, const UString&>&
                            files_listed_signal () const = 0;

    virtual sigc::signal<void,
                         int/*pid*/,
                         const UString&/*target path*/>&
                                            got_target_info_signal () const = 0;

    virtual sigc::signal<void>& running_signal () const=0;

    virtual sigc::signal<void,
                         const UString&/*signal name*/,
                         const UString&/*signal description*/>&
                                            signal_received_signal () const = 0;

    virtual sigc::signal<void, const UString&/*error message*/>&
                                                    error_signal () const = 0;

    virtual sigc::signal<void, IDebugger::State>&
                                        state_changed_signal () const = 0;

    virtual sigc::signal<void,
                         const std::map<register_id_t, UString>&,
                         const UString& >&
                                 register_names_listed_signal () const = 0;

    virtual sigc::signal<void,
                         const std::map<register_id_t, UString>&,
                         const UString& >&
                                   register_values_listed_signal () const = 0;

    virtual sigc::signal<void,
                         const UString&/*register name*/,
                         const UString&/*register value*/,
                         const UString&/*cookie*/>&
                                  register_value_changed_signal () const = 0;

    virtual sigc::signal<void,
                         const std::list<register_id_t>&,
                         const UString& >&
                                   changed_registers_listed_signal () const = 0;

    virtual sigc::signal <void,
                          size_t,/*start address*/
                          const std::vector<uint8_t>&,/*values*/
                          const UString&>&/*cookie*/
                                 read_memory_signal () const = 0;
    virtual sigc::signal <void,
                          size_t,/*start address*/
                          const std::vector<uint8_t>&,/*values*/
                          const UString& >&
                                 set_memory_signal () const = 0;

    // TODO: export informations about what file is being disassembled,
    // what function, which line (if possible) etc.
    // So that the code receiving the signal can adjust accordingly
    virtual sigc::signal<void,
                         const IDebugger::DisassembleInfo&,
                         const std::list<IDebugger::AsmInstr>&,
                         const UString& /*cookie*/>&
                             instructions_disassembled_signal () const = 0;

    virtual sigc::signal<void, const VariableSafePtr, const UString&>&
                                 variable_created_signal () const = 0;

    virtual sigc::signal<void, const VariableSafePtr, const UString&>&
                                 variable_deleted_signal () const = 0;

    virtual sigc::signal<void, const VariableSafePtr, const UString&>&
                                 variable_unfolded_signal () const = 0;

    virtual sigc::signal<void, const VariableSafePtr, const UString&>&
                                 variable_expression_evaluated_signal () const = 0;

    virtual sigc::signal<void, const VariableList&, const UString&>&
                                changed_variables_signal () const  = 0;

    virtual sigc::signal<void, VariableSafePtr, const UString&>&
                assigned_variable_signal () const = 0;
    /// @}

    virtual void do_init (IConfMgrSafePtr &a_conf_mgr) = 0;

    virtual map<UString, UString>& properties () = 0;

    virtual void set_event_loop_context
                        (const Glib::RefPtr<Glib::MainContext> &) = 0;

    virtual void run_loop_iterations (int a_nb_iters) = 0;

    virtual bool busy () const = 0;

    virtual const UString& get_debugger_full_path () const  = 0;

    virtual void set_solib_prefix_path (const UString &a_name) = 0;

    virtual void load_program (const UString &a_prog,
                               const vector<UString> &a_args,
                               const UString &a_working_dir) = 0;

    virtual void load_program
                (const UString &a_prog,
                 const vector<UString> &a_argv,
                 const UString &working_dir,
                 const vector<UString> &a_source_search_dirs,
                 const UString &a_tty_path="") = 0;

    virtual void load_core_file (const UString &a_prog_file,
                                 const UString &a_core_file) = 0;

    virtual bool attach_to_target (unsigned int a_pid,
                                   const UString &a_tty_path="") = 0;

    virtual bool attach_to_remote_target (const UString &a_host,
                                          int a_port) = 0;

    virtual bool attach_to_remote_target (const UString &a_serial_line) = 0;

    virtual void detach_from_target (const UString &a_cookie="") = 0;

    virtual bool is_attached_to_target () const =0;

    virtual void set_tty_path (const UString &a_tty_path) = 0;

    virtual void add_env_variables (const map<UString, UString> &a_vars) = 0;

    virtual map<UString, UString>& get_env_variables () = 0;

    virtual const UString& get_target_path () = 0;

    virtual void get_target_info (const UString &a_cookie="") = 0;

    virtual ILangTrait& get_language_trait () = 0;

    virtual bool is_variable_editable (const VariableSafePtr a_var) const = 0;

    virtual void do_continue (const UString &a_cookie="") = 0;

    virtual void run (const UString &a_cookie="") = 0;

    virtual IDebugger::State get_state () const = 0;

    virtual int get_current_frame_level () const = 0;

    virtual bool stop_target () = 0;

    virtual void exit_engine () = 0;

    virtual void step_in (const UString &a_cookie="") = 0;

    virtual void step_over (const UString &a_cookie="") = 0;

    virtual void step_out (const UString &a_cookie="") = 0;

    virtual void step_instruction (const UString &a_cookie="") = 0;

    virtual void continue_to_position (const UString &a_path,
                                       gint a_line_num,
                                       const UString &a_cookie="") = 0;

    virtual void set_breakpoint (const UString &a_path,
                                 gint a_line_num,
                                 const UString &a_condition= "",
                                 unsigned a_ignore_count = 0,
                                 const UString &a_cookie = "") = 0;

    virtual void set_breakpoint (const UString &a_func_name,
                                 const UString &a_condition = "",
                                 unsigned a_ignore_count = 0,
                                 const UString &a_cookie = "") = 0;

    virtual void enable_breakpoint (gint a_break_num,
                                    const UString &a_cookie="") = 0;

    virtual void disable_breakpoint (gint a_break_num,
                                     const UString &a_cookie="") = 0;

    virtual void set_breakpoint_ignore_count
                                        (gint a_break_num,
                                         gint a_ignore_count,
                                         const UString &a_cookie = "") = 0;

    virtual void set_breakpoint_condition (gint a_break_num,
                                           const UString &a_condition,
                                           const UString &a_cookie = "") = 0;

    virtual void delete_breakpoint (const UString &a_path,
                                    gint a_line_num,
                                    const UString &a_cookie="") = 0;

    virtual void set_watchpoint (const UString &a_expression,
                                 bool a_write = true,
                                 bool a_read = false,
                                 const UString &a_cookie = "") = 0;

    virtual void set_catch (const UString &a_event,
                            const UString &a_cookie="") = 0;

    virtual void list_breakpoints (const UString &a_cookie="") = 0;

    virtual const map<int, Breakpoint>& get_cached_breakpoints () = 0;

    virtual void choose_function_overload (int a_overload_number,
                                           const UString &a_cookie="") = 0;

    virtual void choose_function_overloads (const vector<int> &a_numbers,
                                            const UString &a_cookie="") = 0;

    virtual void delete_breakpoint (gint a_break_num,
                                    const UString &a_cookie="") = 0;

    virtual void list_threads (const UString &a_cookie="") = 0;

    virtual void select_thread (unsigned int a_thread_id,
                                const UString &a_cookie="") = 0;

    virtual unsigned int get_current_thread () const = 0;

    virtual void select_frame (int a_frame_id,
                               const UString &a_cookie="") = 0;

    virtual void list_frames (int a_low_frame=-1,
                              int a_high_frame=-1,
                              const UString &a_cookie="") = 0;

    virtual void list_frames_arguments (int a_low_frame=-1,
                                        int a_high_frame=-1,
                                        const UString &a_cookie="") = 0;

    virtual void list_local_variables (const UString &a_cookie="") = 0;

    virtual void list_global_variables (const UString &a_cookie="") = 0;

    virtual void evaluate_expression (const UString &a_expr,
                                      const UString &a_cookie="") = 0;

    virtual void call_function (const UString &a_call_expression,
                                const UString &a_cookie="") = 0;

    virtual void print_variable_value (const UString &a_var_name,
                                       const UString &a_cookie="")  = 0;

    virtual void get_variable_value (const VariableSafePtr &a_var,
                                     const UString &a_cookie="")  = 0;

    virtual void print_pointed_variable_value (const UString &a_var_name,
                                               const UString &a_cookie="")=0;

    virtual void print_variable_type (const UString &a_var_name,
                                      const UString &a_cookie="") = 0;

    virtual void get_variable_type (const VariableSafePtr &a_var,
                                    const UString &a_cookie="") = 0;

    virtual bool dereference_variable (const VariableSafePtr &a_var,
                                       const UString &a_cookie="") = 0;

    virtual void list_files (const UString &a_cookie="") = 0;

    virtual void list_register_names (const UString &a_cookie="") = 0;
    virtual void list_changed_registers (const UString &a_cookie="") = 0;
    virtual void list_register_values (const UString &a_cookie="") = 0;
    virtual void list_register_values (std::list<register_id_t> a_registers,
                                       const UString &a_cookie="") = 0;
    virtual void set_register_value (const UString& a_reg_name,
                                     const UString& a_value,
                                     const UString& a_cookie="") = 0;

    virtual void read_memory (size_t a_start_addr, size_t a_num_bytes,
            const UString& a_cookie="") = 0;
    virtual void set_memory (size_t a_addr,
            const std::vector<uint8_t>& a_bytes,
            const UString& a_cookie="") = 0;

    typedef sigc::slot<void,
                       const IDebugger::DisassembleInfo&,
                       const std::list<IDebugger::AsmInstr>& > DisassSlot;

    virtual void disassemble (size_t a_start_addr,
                              bool a_start_addr_relative_to_pc,
                              size_t a_end_addr,
                              bool a_end_addr_relative_to_pc,
                              const UString &a_cookie = "") = 0;

    virtual void disassemble (size_t a_start_addr,
                              bool a_start_addr_relative_to_pc,
                              size_t a_end_addr,
                              bool a_end_addr_relative_to_pc,
                              const DisassSlot &a_slot,
                              const UString &a_cookie = "") = 0;

    virtual void disassemble_lines (const UString &a_file_name,
                                    int a_line_num,
                                    int a_nb_disassembled_lines,
                                    const UString &a_cookie = "") = 0;

    virtual void disassemble_lines (const UString &a_file_name,
                                    int a_line_num,
                                    int a_nb_disassembled_lines,
                                    const DisassSlot &a_slot,
                                    const UString &a_cookie = "") = 0;

    typedef sigc::slot<void, const VariableSafePtr> ConstVariableSlot;
    typedef sigc::slot<void, const VariableList> ConstVariableListSlot;
    typedef sigc::slot<void, const UString&> ConstUStringSlot;

    virtual void create_variable (const UString &a_name,
                                  const UString &a_cookie = "") = 0;

    virtual void create_variable (const UString &a_name,
                                  const ConstVariableSlot &a_slot,
                                  const UString &a_cookie = "") = 0;

    virtual void delete_variable (const VariableSafePtr a_var,
                                  const UString &a_cookie = "") = 0;

    virtual void delete_variable (const VariableSafePtr a_var,
                                  const ConstVariableSlot&,
                                  const UString &a_cookie = "") = 0;

    virtual void unfold_variable (VariableSafePtr a_var,
                                  const UString &a_cookie = "") = 0;
    virtual void unfold_variable
                (VariableSafePtr a_var,
                 const ConstVariableSlot&,
                 const UString &a_cookie = "") = 0;

    virtual void assign_variable (const VariableSafePtr a_var,
                                  const UString &a_expression,
                                  const UString &a_cookie = "") = 0;

    virtual void assign_variable
                    (const VariableSafePtr a_var,
                     const UString &a_expression,
                     const ConstVariableSlot &a_slot,
                     const UString &a_cookie="") = 0;

    virtual void evaluate_variable_expr (const VariableSafePtr a_var,
                                         const UString &a_cookie = "") = 0;
    virtual void evaluate_variable_expr
            (const VariableSafePtr a_var,
             const ConstVariableSlot &a_slot,
             const UString &a_cookie = "")= 0;

    virtual void list_changed_variables (VariableSafePtr a_root,
                                         const UString &a_cookie = "") = 0;
    virtual void list_changed_variables
            (VariableSafePtr a_root,
             const ConstVariableListSlot &a_slot,
             const UString &a_cookie="") = 0;

    virtual void query_variable_path_expr (const VariableSafePtr a_var,
                                           const UString &a_cookie = "") = 0;
    virtual void query_variable_path_expr (const VariableSafePtr a_var,
                                           const ConstVariableSlot &a_slot,
                                           const UString &a_cookie = "") = 0;

};//end IDebugger

NEMIVER_END_NAMESPACE (nemiver)

#endif //__NEMIVER_I_DEBUGGER_H__

