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

#include <vector>
#include <glib/gi18n.h>
#include <gtkmm/aboutdialog.h>
//#include "nmv-env.h"
#include "nmv-exception.h"
#include "nmv-plugin.h"
#include "nmv-ui-utils.h"
#include "nmv-i-workbench.h"
#include "nmv-i-perspective.h"
#include "nmv-i-conf-mgr.h"
#include <libgnomevfs/gnome-vfs-init.h>
#include "nmv-log-stream-utils.h"

using namespace std ;
using namespace nemiver ;
using namespace nemiver::common ;
namespace nemiver {

static const UString CONF_KEY_NEMIVER_WINDOW_WIDTH =
                "/apps/nemiver/workbench/window-width" ;
static const UString CONF_KEY_NEMIVER_WINDOW_HEIGHT =
                "/apps/nemiver/workbench/window-height" ;
static const UString CONF_KEY_NEMIVER_WINDOW_POSITION_X =
                "/apps/nemiver/workbench/window-position-x" ;
static const UString CONF_KEY_NEMIVER_WINDOW_POSITION_Y =
                "/apps/nemiver/workbench/window-position-y" ;
static const UString CONF_KEY_NEMIVER_WINDOW_MAXIMIZED =
                "/apps/nemiver/workbench/window-maximized" ;
static const UString CONF_KEY_NEMIVER_WINDOW_MINIMUM_WIDTH=
                "/apps/nemiver/workbench/window-minimum-width" ;
static const UString CONF_KEY_NEMIVER_WINDOW_MINIMUM_HEIGHT=
                "/apps/nemiver/workbench/window-minimum-height" ;


class WorkbenchStaticInit {
    WorkbenchStaticInit ()
    {
        gnome_vfs_init () ;
    }

    ~WorkbenchStaticInit ()
    {
        gnome_vfs_shutdown () ;
    }

public:
    static void do_init ()
    {
        static WorkbenchStaticInit s_wb_init ;
    }

};//end class WorkbenchStaticInit

class Workbench : public IWorkbench {
    struct Priv ;
    SafePtr<Priv> m_priv ;

    Workbench (const Workbench&) ;
    Workbench& operator= (const Workbench&) ;


private:

    //************************
    //<slots (signal callbacks)>
    //************************
    void on_quit_menu_item_action () ;
    void on_about_menu_item_action () ;
    void on_shutting_down_signal () ;
    //************************
    //</slots (signal callbacks)>
    //************************

    void init_glade () ;
    void init_window () ;
    void init_actions () ;
    void init_menubar () ;
    void init_toolbar () ;
    void init_body () ;
    void add_perspective_toolbars (IPerspectiveSafePtr &a_perspective,
                                   list<Gtk::Widget*> &a_tbs) ;
    void add_perspective_body (IPerspectiveSafePtr &a_perspective,
                               Gtk::Widget *a_body) ;
    bool remove_perspective_body (IPerspectiveSafePtr &a_perspective) ;
    void remove_all_perspective_bodies () ;
    void select_perspective (IPerspectiveSafePtr &a_perspective) ;

    void save_window_geometry () ;


    void do_init ()
    {
        WorkbenchStaticInit::do_init ();
    }
    void shut_down () ;
    bool on_delete_event (GdkEventAny* event) ;

public:
    Workbench () ;
    virtual ~Workbench () ;
    void get_info (Info &a_info) const ;
    void do_init (Gtk::Main &a_main) ;
    Glib::RefPtr<Gtk::ActionGroup> get_default_action_group () ;
    Glib::RefPtr<Gtk::ActionGroup> get_debugger_ready_action_group ();
    Gtk::Widget& get_menubar ();
    Gtk::Notebook& get_toolbar_container ();
    Gtk::Window& get_root_window () ;
    Glib::RefPtr<Gtk::UIManager>& get_ui_manager ()  ;
    IPerspective* get_perspective (const UString &a_name) ;
    IConfMgr& get_configuration_manager ()  ;
    Glib::RefPtr<Glib::MainContext> get_main_context ()  ;
    sigc::signal<void>& shutting_down_signal () ;
};//end class Workbench

struct Workbench::Priv {
    bool initialized ;
    Gtk::Main *main ;
    Glib::RefPtr<Gtk::ActionGroup> default_action_group ;
    Glib::RefPtr<Gtk::UIManager> ui_manager ;
    Glib::RefPtr<Gnome::Glade::Xml> glade ;
    SafePtr <Gtk::Window> root_window ;
    Gtk::Widget *menubar ;
    Gtk::Notebook *toolbar_container;
    Gtk::Notebook *bodies_container ;
    PluginManagerSafePtr plugin_manager ;
    list<IPerspectiveSafePtr> perspectives ;
    map<IPerspective*, int> toolbars_index_map ;
    map<IPerspective*, int> bodies_index_map ;
    map<UString, UString> properties ;
    IConfMgrSafePtr conf_mgr ;
    sigc::signal<void> shutting_down_signal ;

    Priv () :
        initialized (false),
        main (0),
        root_window (0),
        menubar (0),
        toolbar_container (0),
        bodies_container (0)
    {
    }

};//end Workbench::Priv

#ifndef CHECK_WB_INIT
#define CHECK_WB_INIT THROW_IF_FAIL(m_priv->initialized) ;
#endif

//****************
//private methods
//****************

//*********************
//signal slots methods
//*********************
bool
Workbench::on_delete_event (GdkEventAny* a_event)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    NEMIVER_TRY
    // use event so that compilation doesn't fail with -Werror :(
    if (a_event) {}

    // clicking the window manager's X and shutting down the with Quit menu item
    // should do the same thing
    on_quit_menu_item_action () ;
    NEMIVER_CATCH

    //keep propagating
    return false;
}

void
Workbench::on_quit_menu_item_action ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    NEMIVER_TRY

    shut_down () ;

    NEMIVER_CATCH
}

void
Workbench::on_about_menu_item_action ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    NEMIVER_TRY

    Gtk::AboutDialog dialog;
    dialog.set_name (PACKAGE_NAME);
    dialog.set_version (PACKAGE_VERSION);
    dialog.set_comments(_("A GNOME frontend for the GNU Debugger"));

    list<Glib::ustring> authors;
    authors.push_back("Dodji Seketeli <dodji@gnome.org>");
    authors.push_back("Jonathon Jongsma <jjongsma@gnome.org>");
    dialog.set_authors(authors);

    dialog.set_website("http://home.gna.org/nemiver/");
    dialog.set_website_label("Project Website");

    Glib::ustring license =
        "This program is free software; you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation; either version 2 of the License, or\n"
        "(at your option) any later version.\n\n"

        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n\n"

        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the \n"
        "Free Software Foundation, Inc., 59 Temple Place, Suite 330, \n"
        "Boston, MA  02111-1307  USA\n";
    dialog.set_license(license);

    // Translators: change this to your name, separate multiple names with \n
    dialog.set_translator_credits(_("translator-credits"));

    dialog.run ();

    NEMIVER_CATCH
}

void
Workbench::on_shutting_down_signal ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    NEMIVER_TRY
    save_window_geometry () ;
    NEMIVER_CATCH
}

Workbench::Workbench ()
{
    m_priv.reset (new Priv ());
}

Workbench::~Workbench ()
{
    remove_all_perspective_bodies () ;
    LOG_D ("delete", "destructor-domain") ;
}

void
Workbench::get_info (Info &a_info) const
{
    static Info s_info ("workbench",
                        "The workbench of Nemiver",
                        "1.0") ;
    a_info = s_info ;
}

void
Workbench::do_init (Gtk::Main &a_main)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    DynamicModule::Loader *loader = get_module_loader () ;
    THROW_IF_FAIL (loader) ;

    DynamicModuleManager *dynmod_manager = loader->get_dynamic_module_manager () ;
    THROW_IF_FAIL (dynmod_manager) ;

    m_priv->main = &a_main ;

    init_glade () ;
    init_window () ;
    init_actions () ;
    init_menubar () ;
    init_toolbar () ;
    init_body () ;

    m_priv->initialized = true ;

    m_priv->plugin_manager =
        PluginManagerSafePtr (new PluginManager (*dynmod_manager)) ;

    NEMIVER_TRY
        m_priv->plugin_manager->load_plugins () ;

        map<UString, PluginSafePtr>::const_iterator plugin_iter ;
        IPerspectiveSafePtr perspective ;
        Plugin::EntryPointSafePtr entry_point ;
        list<Gtk::Widget*> toolbars ;

        //**************************************************************
        //store the list of perspectives we may have loaded as plugins,
        //and init each of them.
        //**************************************************************
        IWorkbench *workbench = dynamic_cast<IWorkbench*> (this) ;
        for (plugin_iter = m_priv->plugin_manager->plugins_map ().begin () ;
             plugin_iter != m_priv->plugin_manager->plugins_map ().end ();
             ++plugin_iter) {
             LOG_D ("plugin '"
                    << plugin_iter->second->descriptor ()->name ()
                    << "' refcount: "
                    << (int) plugin_iter->second->get_refcount (),
                    "refcount-domain") ;
            if (plugin_iter->second && plugin_iter->second->entry_point_ptr ()) {
                entry_point = plugin_iter->second->entry_point_ptr () ;
                perspective = entry_point.do_dynamic_cast<IPerspective> () ;
                if (perspective) {
                    m_priv->perspectives.push_front (perspective) ;
                    perspective->do_init (workbench) ;
                    perspective->edit_workbench_menu () ;
                    toolbars.clear () ;
                    perspective->get_toolbars (toolbars) ;
                    add_perspective_toolbars (perspective, toolbars) ;
                    add_perspective_body (perspective, perspective->get_body ()) ;
                    LOG_D ("perspective '"
                           << perspective->get_perspective_identifier ()
                           << "' refcount: "
                           << (int) perspective->get_refcount (),
                           "refcount-domain") ;
                }
            }
        }

        if (!m_priv->perspectives.empty ()) {
            select_perspective (*m_priv->perspectives.begin ()) ;
        }
    NEMIVER_CATCH
}

void
Workbench::shut_down ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    shutting_down_signal ().emit () ;
    m_priv->main->quit () ;
}

Glib::RefPtr<Gtk::ActionGroup>
Workbench::get_default_action_group ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    CHECK_WB_INIT ;
    THROW_IF_FAIL (m_priv) ;
    return m_priv->default_action_group ;
}

Gtk::Widget&
Workbench::get_menubar ()
{
    CHECK_WB_INIT ;
    THROW_IF_FAIL (m_priv && m_priv->menubar) ;
    return *m_priv->menubar ;
}

Gtk::Notebook&
Workbench::get_toolbar_container ()
{
    CHECK_WB_INIT ;
    THROW_IF_FAIL (m_priv && m_priv->toolbar_container) ;
    return *m_priv->toolbar_container ;
}

Gtk::Window&
Workbench::get_root_window ()
{
    CHECK_WB_INIT ;
    THROW_IF_FAIL (m_priv && m_priv->root_window) ;

    return *m_priv->root_window ;
}

Glib::RefPtr<Gtk::UIManager>&
Workbench::get_ui_manager ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->ui_manager) {
        m_priv->ui_manager = Gtk::UIManager::create () ;
        THROW_IF_FAIL (m_priv->ui_manager) ;
    }
    return m_priv->ui_manager ;
}

IPerspective*
Workbench::get_perspective (const UString &a_name)
{
    list<IPerspectiveSafePtr>::const_iterator iter ;
    for (iter = m_priv->perspectives.begin ();
         iter != m_priv->perspectives.end ();
         ++iter) {
        if ((*iter)->descriptor ()->name () == a_name) {
            return iter->get () ;
        }
    }
    LOG_ERROR ("could not find perspective: '" << a_name << "'") ;
    return NULL;
}

IConfMgr&
Workbench::get_configuration_manager ()
{
    THROW_IF_FAIL (m_priv) ;
    if (!m_priv->conf_mgr) {
        DynamicModule::Loader *loader = get_module_loader () ;
        THROW_IF_FAIL (loader) ;

        DynamicModuleManager *dynmod_manager =
                loader->get_dynamic_module_manager () ;
        THROW_IF_FAIL (dynmod_manager) ;

        m_priv->conf_mgr = dynmod_manager->load<IConfMgr> ("gconfmgr") ;
        m_priv->conf_mgr->do_init () ;
        m_priv->conf_mgr->set_key_dir_to_notify ("/apps/nemiver") ;
    }
    THROW_IF_FAIL (m_priv->conf_mgr) ;
    return *m_priv->conf_mgr ;
}

Glib::RefPtr<Glib::MainContext>
Workbench::get_main_context ()
{
    THROW_IF_FAIL (m_priv) ;
    return Glib::MainContext::get_default () ;
}

sigc::signal<void>&
Workbench::shutting_down_signal ()
{
    THROW_IF_FAIL (m_priv) ;

    return m_priv->shutting_down_signal ;
}

void
Workbench::init_glade ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;

    UString file_path = env::build_path_to_glade_file ("workbench.glade") ;
    m_priv->glade = Gnome::Glade::Xml::create (file_path) ;
    THROW_IF_FAIL (m_priv->glade) ;

    m_priv->root_window.reset
           (ui_utils::get_widget_from_glade<Gtk::Window> (m_priv->glade,
                                                          "workbench"));
   // m_priv->root_window->property_allow_shrink ().set_value (true) ;
    m_priv->root_window->hide () ;
}
void

Workbench::init_window ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->root_window) ;
    IConfMgr &conf_mgr = get_configuration_manager () ;

    int width=0, height=0, pos_x=0, pos_y=0 ;
    LOG_DD ("getting windows geometry from confmgr ...") ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_WIDTH, width) ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_HEIGHT, height) ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_X, pos_x) ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_Y, pos_y) ;
    bool maximized=false ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_MAXIMIZED, maximized) ;
    LOG_DD ("got windows geometry from confmgr.") ;

    if (width) {
        LOG_DD ("restoring windows geometry from confmgr ...") ;
        m_priv->root_window->resize (width, height) ;
        m_priv->root_window->move (pos_x, pos_y) ;
        if (maximized) {
            m_priv->root_window->maximize () ;
        }
        LOG_DD ("restored windows geometry from confmgr") ;
    } else {
        LOG_DD ("null window geometry from confmgr.") ;
    }

    //set the minimum width/height of nemiver, just in case.
    width=700, height=500 ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_MINIMUM_WIDTH, width) ;
    conf_mgr.get_key_value (CONF_KEY_NEMIVER_WINDOW_MINIMUM_HEIGHT, height) ;
    m_priv->root_window->set_size_request (width, height) ;
    LOG_DD ("set windows min size to ("
            << (int) width
            << ","
            << (int) height
            << ")") ;

    shutting_down_signal ().connect (sigc::mem_fun
                        (*this, &Workbench::on_shutting_down_signal)) ;
    m_priv->root_window->signal_delete_event ().connect (
            sigc::mem_fun (*this, &Workbench::on_delete_event));
}

void
Workbench::init_actions ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    Gtk::StockID nil_stock_id ("") ;
    sigc::slot<void> nil_slot ;
    using ui_utils::ActionEntry ;

    static ActionEntry s_default_action_entries [] = {
        {
            "FileMenuAction",
            nil_stock_id,
            _("_File"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            ""
        }
        ,
        {
            "QuitMenuItemAction",
            Gtk::Stock::QUIT,
            _("_Quit"),
            _("Quit the application"),
            sigc::mem_fun (*this, &Workbench::on_quit_menu_item_action),
            ActionEntry::DEFAULT,
            ""
        }
        ,
        {
            "EditMenuAction",
            nil_stock_id,
            _("_Edit"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            ""
        }
        ,
        {
            "HelpMenuAction",
            nil_stock_id,
            _("_Help"),
            "",
            nil_slot,
            ActionEntry::DEFAULT,
            ""
        }
        ,
        {
            "AboutMenuItemAction",
            Gtk::Stock::ABOUT,
            _("_About"),
            _("Display information about this application"),
            sigc::mem_fun (*this, &Workbench::on_about_menu_item_action),
            ActionEntry::DEFAULT,
            ""
        }
    };

    m_priv->default_action_group =
        Gtk::ActionGroup::create ("workbench-default-action-group") ;
    int num_default_actions =
         sizeof (s_default_action_entries)/sizeof (ui_utils::ActionEntry) ;
    ui_utils::add_action_entries_to_action_group (s_default_action_entries,
                                                  num_default_actions,
                                                  m_priv->default_action_group) ;
    get_ui_manager ()->insert_action_group (m_priv->default_action_group) ;
}

void
Workbench::init_menubar ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv && m_priv->default_action_group) ;


    UString file_path = env::build_path_to_menu_file ("menubar.xml") ;
    m_priv->ui_manager->add_ui_from_file (file_path) ;

    m_priv->menubar = m_priv->ui_manager->get_widget ("/MenuBar") ;
    THROW_IF_FAIL (m_priv->menubar) ;

    Gtk::Box *menu_container =
        ui_utils::get_widget_from_glade<Gtk::Box> (m_priv->glade, "menucontainer");
    menu_container->pack_start (*m_priv->menubar) ;
    menu_container->show_all () ;
}

void
Workbench::init_toolbar ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    m_priv->toolbar_container =
        ui_utils::get_widget_from_glade<Gtk::Notebook> (m_priv->glade,
                                                        "toolbarcontainer") ;
}

void
Workbench::init_body ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;
    m_priv->bodies_container =
        ui_utils::get_widget_from_glade<Gtk::Notebook> (m_priv->glade,
                                                        "bodynotebook") ;
}

void
Workbench::add_perspective_toolbars (IPerspectiveSafePtr &a_perspective,
                                     list<Gtk::Widget*> &a_tbs)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    if (a_tbs.empty ()) {return ;}

    SafePtr<Gtk::Box> box (Gtk::manage (new Gtk::VBox)) ;
    list<Gtk::Widget*>::const_iterator iter ;

    for (iter = a_tbs.begin (); iter != a_tbs.end () ; ++iter) {
        box->pack_start (**iter) ;
    }

    box->show_all () ;
    m_priv->toolbars_index_map [a_perspective.get ()] =
                    m_priv->toolbar_container->insert_page (*box, -1) ;

    box.release () ;
}

void
Workbench::add_perspective_body (IPerspectiveSafePtr &a_perspective,
                                 Gtk::Widget *a_body)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    if (!a_body || !a_perspective) {return;}

    m_priv->bodies_index_map[a_perspective.get ()] =
        m_priv->bodies_container->insert_page (*a_body, -1);
}

bool
Workbench::remove_perspective_body (IPerspectiveSafePtr &a_perspective)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->bodies_container) ;

    if (!a_perspective) {return false;}

    map<IPerspective*, int>::iterator it;
    it = m_priv->bodies_index_map.find (a_perspective.get ()) ;
    if (it == m_priv->bodies_index_map.end ()) {
        return false ;
    }
    m_priv->bodies_container->remove_page (it->second)  ;
    m_priv->bodies_index_map.erase (it) ;
    return true ;
}

void
Workbench::save_window_geometry () 
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->root_window) ;
    IConfMgr &conf_mgr = get_configuration_manager () ;

    int width=0, height=0, pos_x=0, pos_y=0 ;
    m_priv->root_window->get_size (width, height) ;
    m_priv->root_window->get_position (pos_x, pos_y) ;
    bool maximized = (m_priv->root_window->get_window()->get_state()
                      & Gdk::WINDOW_STATE_MAXIMIZED);

    conf_mgr.set_key_value (CONF_KEY_NEMIVER_WINDOW_MAXIMIZED, maximized) ;
    if (!maximized) {
        LOG_DD ("storing windows geometry to confmgr...") ;
        conf_mgr.set_key_value (CONF_KEY_NEMIVER_WINDOW_WIDTH, width) ;
        conf_mgr.set_key_value (CONF_KEY_NEMIVER_WINDOW_HEIGHT, height) ;
        conf_mgr.set_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_X, pos_x) ;
        conf_mgr.set_key_value (CONF_KEY_NEMIVER_WINDOW_POSITION_Y, pos_y) ;
        LOG_DD ("windows geometry stored to confmgr") ;
    } else {
        LOG_DD ("windows was maximized, didn't store its geometry") ;
    }
}

void
Workbench::remove_all_perspective_bodies ()
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    map<IPerspective*, int>::iterator it ;
    for (it = m_priv->bodies_index_map.begin ();
         it != m_priv->bodies_index_map.end ();
         ++it) {
        m_priv->bodies_container->remove_page (it->second) ;
    }
    m_priv->bodies_index_map.clear () ;
}

void
Workbench::select_perspective (IPerspectiveSafePtr &a_perspective)
{
    LOG_FUNCTION_SCOPE_NORMAL_DD ;

    THROW_IF_FAIL (m_priv) ;
    THROW_IF_FAIL (m_priv->toolbar_container) ;
    THROW_IF_FAIL (m_priv->bodies_container) ;

    map<IPerspective*, int>::const_iterator iter, nil ;
    int toolbar_index=0, body_index=0 ;

    nil = m_priv->toolbars_index_map.end () ;
    iter = m_priv->toolbars_index_map.find (a_perspective.get ()) ;
    if (iter != nil) {
        toolbar_index = iter->second ;
    }

    nil = m_priv->bodies_index_map.end () ;
    iter = m_priv->bodies_index_map.find (a_perspective.get ()) ;
    if (iter != nil) {
        body_index = iter->second ;
    }

    m_priv->toolbar_container->set_current_page (toolbar_index) ;

    m_priv->bodies_container->set_current_page (body_index) ;
}


//the dynmod initial factory.
extern "C" {
bool
NEMIVER_API nemiver_common_create_dynamic_module_instance (void **a_new_instance)
{
    *a_new_instance = new nemiver::Workbench () ;
    return (*a_new_instance != 0) ;
}

}//end extern C

}//end namespace nemiver

