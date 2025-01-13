#include <gtkmm.h>
#include "MainWindow.hpp"
#include <iostream>

namespace {
    Gtk::ApplicationWindow* win = nullptr;
    Glib::RefPtr<Gtk::Application> app;

    void on_app_activate(){
    
        win = BuildWindow();

        win->signal_hide().connect([] () { delete win; });

        app->add_window(*win);
        win->set_visible(true);
    }
}

int main(int argc, char** argv) {
    app = Gtk::Application::create("org.veebs.gctools");

    // Instantiate a dialog when the application has been activated.
    // This can only be done after the application has been registered.
    // It's possible to call app->register_application() explicitly, but
    // usually it's easier to let app->run() do it for you.
    app->signal_activate().connect([] () { on_app_activate(); });

    return app->run(argc, argv);
}