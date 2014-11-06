#ifndef cclust_pthread
#define cclust_pthread

#include "cclust.h"
#include <iostream>
#include <glibmm.h>

template <typename T>
class preprocess_cclust_thread{
private:
  Glib::Thread *thread;
  uint number_of_runs;

  // TODO: mutex these
  vector<clustering<T> > *clusterings;
  clustering<T> *consensus;

  // maybe a mutex for this one? - maybe not.. we only read it
  const bool *cancel_computation;


  double *progress_pc;
  Glib::Dispatcher *disp_computation_done;

  // ==================================================
	void run(){
    // apply preprocessing at most 'number_of_runs' times
    uint old_clustered;
    uint new_clustered = 0;
    for(uint i = 0; i < number_of_runs; i++){
      old_clustered = new_clustered;
      *consensus = apply_preprocessing<T>(*clusterings, *consensus,
                                          progress_pc, cancel_computation);
      new_clustered = get_clustered_elements(*consensus).size();
      if(old_clustered == new_clustered) break;
    }
    disp_computation_done->emit();
  }

public:
	preprocess_cclust_thread(vector<clustering<T> > *_clusterings,
                      clustering<T> *_consensus,
                      const uint nr_runs,
                      const bool* cancel_comp,
                      double *_progress_pc,
                      Glib::Dispatcher *comp_done)
    :number_of_runs(nr_runs),clusterings(_clusterings),
    consensus(_consensus),cancel_computation(cancel_comp),
    progress_pc(_progress_pc),disp_computation_done(comp_done){}

	void start(){
    // create a joinable thread
    thread = Glib::Thread::create(sigc::mem_fun(*this, &preprocess_cclust_thread::run), true);
  }
	void wait(){
    return thread->join();
  }
};




template <typename T>
class searchtree_cclust_thread{
private:
  Glib::Thread *thread;

  // TODO: mutex these
  vector<clustering<T> > *clusterings;
  clustering<T> *consensus;

  // maybe a mutex for this one? - maybe not.. we only read it
  const bool *cancel_computation;

  double *progress_pc;
  Glib::Dispatcher *disp_computation_done;

  // ==================================================
	void run(){
    // do brute force search
    *consensus =
      get_consensus_clustering_brute(*clusterings, *consensus,
          progress_pc, cancel_computation);
    disp_computation_done->emit();
  }

public:
	searchtree_cclust_thread(vector<clustering<T> > *_clusterings,
                      clustering<T> *_consensus,
                      const bool* cancel_comp,
                      double *_progress_pc,
                      Glib::Dispatcher *comp_done)
    :clusterings(_clusterings), consensus(_consensus),
    cancel_computation(cancel_comp), progress_pc(_progress_pc),
    disp_computation_done(comp_done){}

	void start(){
    // create a joinable thread
    thread = Glib::Thread::create(sigc::mem_fun(*this, &searchtree_cclust_thread::run), true);
  }
	void wait(){
    return thread->join();
  }
};

#endif
