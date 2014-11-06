#ifndef GLOBALS_HPP_INCLUDED
#define GLOBALS_HPP_INCLUDED

#include <string>
#include <iostream>
#include <cstdlib>
#include <unistd.h>


#ifndef DATADIR
#define DATADIR "."
#endif // DATADIR

#ifndef NODEBUG
#define DEBUG(x) std::cout << x;
#else
#define DEBUG(x) ;
#endif

inline const bool file_accessible(const std::string& name) {
  return (access( name.c_str(), R_OK ) == 0 );
}


inline const std::string find_file(const std::string& name){
  if(file_accessible(name)) return name; else{
    std::string s(DATADIR + name);
    if(file_accessible(s)) return s; else {
      std::cerr << "file "<<name<<" could not be found or read (tried "<<name<<" and "<<s<<")"<<std::endl;
      exit(1);
    }
  }
}

#endif // GLOBALS_HPP_INCLUDED
