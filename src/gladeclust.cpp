
#include "gcclust_window.hpp"

int main(int argc,char **argv){
  Gtk::Main gcclust(argc, argv);


  gcclust_window main_window;


  Gtk::Main::run(*main_window.window);

  return(0);
}
