#ifndef timer_h
#define timer_h

#include <glibmm.h>

class timer{
private:
  Glib::Thread* thread;
  uint interval_usec;
  Glib::Dispatcher *interval_done;

  bool execute_timer;

  // ==================================================
	void run(){
   while(execute_timer){
      Glib::usleep(interval_usec);
      interval_done->emit();
    }
  }

public:
  timer(const uint _interval_usec,
        Glib::Dispatcher *_interval_done)
  :interval_usec(_interval_usec), interval_done(_interval_done), execute_timer(false){}

	void start(){
    execute_timer = true;
    thread = Glib::Thread::create(sigc::mem_fun(*this, &timer::run), true);
  }
  void stop(){
    execute_timer = false;
  }
	void wait(){
    thread->join();
  }
};
#endif
