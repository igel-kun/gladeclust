

#ifndef GCCLUST_WINDOW_HH
#define GCCLUST_WINDOW_HH

#include <gtkmm.h>
#include "timer.h"
#include "cclust_pthread.h"
#include "edit_clusterings.hpp"

class gcclust_window
{
  // random stuff
  std::string clusterings_filename;
  std::vector<clustering<std::string> > clusterings;

  std::string consensus_filename;
  clustering<std::string> consensus;

  // Edit Clusterings window
  edit_clusterings_window* EditClusterings;

  // a thread to solve the instances in the background
  preprocess_cclust_thread<std::string> *preprocess_thread;
  searchtree_cclust_thread<std::string> *searchtree_thread;

  // dispatcher for showing progress and changing labels and treeviews
  Glib::Dispatcher signal_progress_pc;
  Glib::Dispatcher signal_computation_done;
  sigc::connection comp_done_con;

  double progress_pc;
  // a thread to update the progress bar from progress_pc every 100us
  timer timer_thread;

  // treeview stuff
  class ClusteringColumns : public Gtk::TreeModel::ColumnRecord{
    // TODO: add a "distance" column
  	public:
  	ClusteringColumns(){ add(m_col_text); add(m_col_clustering);}
  	Gtk::TreeModelColumn<std::string> m_col_text;
    // this is hidden data, because it is not being added as a view column
  	Gtk::TreeModelColumn<const clustering<std::string>* > m_col_clustering;
  };
  ClusteringColumns model_Columns_clusterings;
  Glib::RefPtr<Gtk::ListStore> pClusteringsList;
  Glib::RefPtr<Gtk::ListStore> pConsensusList;

  // update the treeview using the clusterings vector
  void update_tvClusterings();
  void update_tvConsensus();
  std::string select_a_file(const std::string caption,
      const Gtk::FileChooserAction action = Gtk::FILE_CHOOSER_ACTION_OPEN);
  void on_edit_complete();

  // measuring the time
  clock_t start_time;
  bool measure_time;

  bool cancel_computation;
  void update_percent();
  void preprocess_complete();
  void searchtree_complete();
  void brute_start(const clustering<std::string> &cons);
  bool calc_avg_dist_when_idle();


  // ========== window elements we use ==================
  public:
    Gtk::Window*        window;
  private:
    Gtk::Label*         lblClusteringsFrame;
    Gtk::Label*         lblConsensusFrame;
    Gtk::Label*         lblProgress;
    Gtk::ProgressBar*   pgbProgress;
    Gtk::CheckMenuItem* preprocess_clusterings1;
    Gtk::CheckMenuItem* apply_exhaustively1;
    Gtk::CheckMenuItem* measure_time1;
    Gtk::ImageMenuItem* cancel1;
    Gtk::TreeView*      tvClusterings;
    Gtk::TreeView*      tvConsensus;

    Gtk::MenuItem*      about1;
    Gtk::ImageMenuItem* quit1;
    Gtk::ImageMenuItem* new1;
    Gtk::ImageMenuItem* edit1;
    Gtk::ImageMenuItem* open1;
    Gtk::ImageMenuItem* generate_randomly1;
    Gtk::ImageMenuItem* clusterings_save1;
    Gtk::ImageMenuItem* clusterings_save_as1;
    Gtk::ImageMenuItem* consensus_save1;
    Gtk::ImageMenuItem* consensus_save_as1;
    Gtk::CheckMenuItem* brute_force_search1;
    Gtk::ImageMenuItem* compute_consensus1;

  private:
    void get_my_elements(Glib::RefPtr<Gtk::Builder> builder);
    void connect_my_signals();

    // ============ signal handlers ====================
    void on_about1_activate();
    void on_quit1_activate();
    void on_new1_activate();
    void on_edit1_activate();
    void on_open1_activate();
    void on_generate_randomly1_activate();
    void on_clusterings_save1_activate();
    void on_clusterings_save_as1_activate();
    void on_consensus_save1_activate();
    void on_consensus_save_as1_activate();
    void on_preprocess_clusterings1_activate();
    void on_brute_force_search1_activate();
    void on_measure_time1_activate();
    void on_compute_consensus1_activate();
    void on_cancel1_activate();
    bool on_gcclust_window_configure_event(GdkEventConfigure *ev);

  public:
    // constructors & descructors
    gcclust_window();
    ~gcclust_window();

};
#endif
