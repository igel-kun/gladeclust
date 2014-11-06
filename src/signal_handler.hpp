#ifndef SIGNAL_HANDLER_HPP_INCLUDED
#define SIGNAL_HANDLER_HPP_INCLUDED

#include "globals.hpp"
#include "gladeclust.hpp"

extern "C" {


  bool on_gcclust_window_configure_event(GdkEventConfigure *ev);
  void on_compute_consensus1_activate();
  void preprocess_complete();
  void searchtree_complete();
  void update_tvConsensus();
  void update_tvClusterings();
  void update_percent();
  bool calc_avg_dist_when_idle();
  void on_measure_time1_activate();
  void on_preprocess_clusterings1_activate();
  void on_brute_force_search1_activate();
  void on_quit1_activate();
  void on_new1_activate();
  void on_cancel1_activate();
  void on_consensus_save1_activate();
  void on_consensus_save_as1_activate();
  void on_edit1_activate();
  void on_edit_complete();
  void on_open1_activate();
  void on_generate_randomly1_activate();
  void on_clusterings_save1_activate();
  void on_clusterings_save_as1_activate();
}








bool on_gcclust_window_configure_event(GdkEventConfigure *ev)
{
  Gtk::Label* lblClusteringsFrame;
  builder->get_widget("lblClusteringsFrame", lblClusteringsFrame);
  Gtk::CheckMenuItem* preprocess_clusterings1;
  builder->get_widget("preprocess_clusterings1", preprocess_clusterings1);
  Gtk::CheckMenuItem* apply_exhaustively1;
  builder->get_widget("apply_exhaustively1", apply_exhaustively1);
  Gtk::CheckMenuItem* measure_time1;
  builder->get_widget("measure_time1", measure_time1);
  Gtk::ImageMenuItem* cancel1;
  builder->get_widget("cancel1", cancel1);
  Gtk::TreeView* tvClusterings;
  builder->get_widget("tvClusterings", tvClusterings);
  Gtk::TreeView* tvConsensus;
  builder->get_widget("tvConsensus", tvConsensus);

  pClusteringsList = Gtk::ListStore::create(model_Columns_clusterings);
  tvClusterings->set_model(pClusteringsList);
  tvClusterings->append_column("Clustering", model_Columns_clusterings.m_col_text);

  pConsensusList = Gtk::ListStore::create(model_Columns_clusterings);
  tvConsensus->set_model(pConsensusList);
  tvConsensus->append_column("Consensus", model_Columns_clusterings.m_col_text);

  lblClusteringsFrame->set_label("input Clusterings (average distance: ...)");

  // connect the progress bar update routine to the appropriate dispatcher
  signal_progress_pc.connect(sigc::ptr_fun(&update_percent));

  // time measurement
  measure_time = measure_time1->get_active();
  // threading
  preprocess_thread = NULL;
  searchtree_thread = NULL;
  // misc stuff
  cancel1->set_sensitive(false);
  // this will automtically update the tvConsensus as well
  update_tvClusterings();

  return false;
}


void on_compute_consensus1_activate()
{
  Gtk::Label* lblProgress;
  builder->get_widget("lblProgress", lblProgress);
  Gtk::CheckMenuItem* preprocess_clusterings1;
  builder->get_widget("preprocess_clusterings1", preprocess_clusterings1);
  Gtk::CheckMenuItem* apply_exhaustively1;
  builder->get_widget("apply_exhaustively1", apply_exhaustively1);
  Gtk::ImageMenuItem* cancel1;
  builder->get_widget("cancel1", cancel1);


  lblProgress->set_text("preprocess:");

  uint preprocessing = 0;

  if(preprocess_clusterings1->get_active())
    preprocessing = (apply_exhaustively1->get_active()?(uint)(-1):1);

  DEBUG("preprocessing " << preprocessing << " times" << std::endl);

  comp_done_con.disconnect();
  comp_done_con = signal_computation_done.connect(
      sigc::ptr_fun(&preprocess_complete));

  if(preprocess_thread) delete preprocess_thread;
  preprocess_thread = new preprocess_cclust_thread<std::string>(&clusterings, &consensus,
      preprocessing, &cancel_computation, &progress_pc, &signal_computation_done);

  cancel_computation = false;
  cancel1->set_sensitive(true);

  // take the time for measuring
  start_time = clock();

  // start the thread for updating the progress bar
  timer_thread.start();

  // start the actual computation
  preprocess_thread->start();
}



// the preprocessing is complete, this is a callback function of the
// preprocess_thread
void preprocess_complete(){
  DEBUG("preprocessing complete" << std::endl);

  Gtk::ImageMenuItem* cancel1;
  builder->get_widget("cancel1", cancel1);
  Gtk::Label* lblProgress;
  builder->get_widget("lblProgress", lblProgress);
  Gtk::CheckMenuItem* brute_force_search1;
  builder->get_widget("brute_force_search1", brute_force_search1);

  cancel1->set_sensitive(false);
  cancel_computation = false;

  // if the 'brute force' option is selected, start the searchtree_thread
  if(brute_force_search1->get_active()){
    lblProgress->set_label("brute force:");

	  comp_done_con.disconnect();
	  comp_done_con = signal_computation_done.connect(
	      sigc::ptr_fun(&searchtree_complete));

	  if(searchtree_thread) delete searchtree_thread;
	  searchtree_thread = new searchtree_cclust_thread<std::string>(&clusterings, &consensus,
	      &cancel_computation, &progress_pc, &signal_computation_done);

	  cancel1->set_sensitive(true);
	  searchtree_thread->start();
  } else {
    // wait till the timer thread finishes up
    timer_thread.stop();
    timer_thread.wait();

    // measure the computation time
    clock_t stop_time;
    if(measure_time) stop_time = clock();

    // update the consensus view
    update_tvConsensus();

    // show a dialog informing the user about the computation time
    if(measure_time){
      std::stringstream s;

      s.precision(4);
      s.str(std::string());
      s << "computation took " << ((double)(stop_time - start_time))/CLOCKS_PER_SEC << " seconds";
      Gtk::MessageDialog msg(s.str());
      msg.run();
    }

  }
}


// a solution is found, the search tree thread is returning
// to this function as a callback
void searchtree_complete(){
  Gtk::ImageMenuItem* cancel1;
  builder->get_widget("cancel1", cancel1);
  Gtk::Label* lblProgress;
  builder->get_widget("lblProgress", lblProgress);

  DEBUG("computation complete" << std::endl);
  cancel1->set_sensitive(false);
  cancel_computation = false;
  lblProgress->set_label("done");

  // wait till the timer thread finishes up
  timer_thread.stop();
  timer_thread.wait();

  // measure the computation time
  clock_t stop_time;
  if(measure_time) stop_time = clock();

  // update the consensus view
  update_tvConsensus();

  // show a dialog informing the user about the computation time
  if(measure_time){
      std::stringstream s;

      s.precision(4);
      s.str(std::string());
      s << "computation took " << ((double)(stop_time - start_time))/CLOCKS_PER_SEC << " seconds";
      Gtk::MessageDialog msg(s.str());
      msg.run();
  }
}



void update_tvConsensus(){
  Gtk::ImageMenuItem* consensus_save1;
  builder->get_widget("consensus_save1", consensus_save1);
  Gtk::ImageMenuItem* consensus_save_as1;
  builder->get_widget("consensus_save_as1", consensus_save_as1);
  Gtk::Label* lblConsensusFrame;
  builder->get_widget("lblConsensusFrame", lblConsensusFrame);

  std::stringstream s;
  // clear all rows
  pConsensusList->clear();

  // redisplay the consensus clustering
  // add row
  Gtk::TreeModel::Row row = *(pConsensusList->append());

  // set values
  s.str(std::string());
  s << consensus;
  row[model_Columns_clusterings.m_col_text] = s.str();
  row[model_Columns_clusterings.m_col_clustering] = &consensus;

  if(consensus == clustering<string>()){
    // disable consensus saving
    consensus_save1->set_sensitive(false);
    consensus_save_as1->set_sensitive(false);
  } else {
    // enable consensus saving
    consensus_save1->set_sensitive(true);
    consensus_save_as1->set_sensitive(true);
  }


  // if there are too many clusterings or too many elements, this computation takes
  // way too much time
  if(clusterings.size() && (clusterings.size()<200) && (clusterings.begin()->size()<200)){
    s.str(std::string());
    s.precision(4);
    s << "consensus Clusterings (cumulative distance: " << get_distance(consensus,clusterings) << ")";
    lblConsensusFrame->set_label(s.str());
  } else lblConsensusFrame->set_label("consensus Clustering");
}



void update_tvClusterings(){
  Gtk::Label* lblClusteringsFrame;
  builder->get_widget("lblClusteringsFrame", lblClusteringsFrame);
  Gtk::ImageMenuItem* clusterings_save1;
  builder->get_widget("clusterings_save1", clusterings_save1);
  Gtk::ImageMenuItem* clusterings_save_as1;
  builder->get_widget("clusterings_save_as1", clusterings_save_as1);

  std::stringstream s;
  // clear all rows
  pClusteringsList->clear();

  // redisplay all clusterings
  for(vector<clustering<std::string> >::const_iterator i = clusterings.begin();
      i != clusterings.end(); i++){
    // add row
    Gtk::TreeModel::Row row = *(pClusteringsList->append());

    // set values
    s.str(std::string());
    s << *i;
    row[model_Columns_clusterings.m_col_text] = s.str();
    row[model_Columns_clusterings.m_col_clustering] = &(*i);
  }

  if(clusterings.size()){
    // enable consensus saving
    clusterings_save1->set_sensitive(true);
    clusterings_save_as1->set_sensitive(true);
  } else {
    // disable consensus saving
    clusterings_save1->set_sensitive(false);
    clusterings_save_as1->set_sensitive(false);
  }

  // if the clusterings changed, then the consensus has to be recalculated
  consensus = clustering<string>();
  update_tvConsensus();

  lblClusteringsFrame->set_label("input Clusterings (average distance: ?)");
  Glib::signal_idle().connect(sigc::ptr_fun(&calc_avg_dist_when_idle));
}


// this function is called every 100us by the timer thread
void update_percent(){
  Gtk::ProgressBar* pgbProgress;
  builder->get_widget("pgbProgress", pgbProgress);

  stringstream s;
  s.precision(4);
  s << 100*progress_pc << "\%";
  pgbProgress->set_text(s.str());
  pgbProgress->set_fraction(progress_pc);
}


// when idle, calculate the average distance of the input clusterings,
// if they are less then 200 clusterings and less than 200 elements
bool calc_avg_dist_when_idle(){
  if(clusterings.size() && (clusterings.size()<200) && (clusterings.begin()->size()<200)){
    DEBUG("we are idle, lets calc avg distances (" << clusterings.size() << " clusterings, " << clusterings.begin()->size() << "elements)..." << std::endl);

    Gtk::Label* lblClusteringsFrame;
    builder->get_widget("lblClusteringsFrame", lblClusteringsFrame);

    stringstream s;
    s.precision(4);
    s << "input Clusterings (average distance: " << get_avg_distance(clusterings) << ")";
    lblClusteringsFrame->set_label(s.str());
    // remove the signal handler
  }
  return false;
}


void on_measure_time1_activate()
{
  Gtk::CheckMenuItem* measure_time1;
  builder->get_widget("measure_time1", measure_time1);

  measure_time = measure_time1->get_active();
}

void on_preprocess_clusterings1_activate()
{
}

void on_brute_force_search1_activate()
{
}


void on_quit1_activate()
{
  Gtk::Main::quit();
}

void on_new1_activate()
{
  clusterings.clear();
  update_tvClusterings();
}


void on_cancel1_activate()
{
  cancel_computation = true;
}

void on_consensus_save1_activate()
{
  if(consensus_filename.size()){
    write_clustering_to_file<std::string>(consensus_filename, consensus);
  } else on_consensus_save_as1_activate();
}

void on_consensus_save_as1_activate()
{
  consensus_filename = select_a_file("please select a file to save the consensus clustering to",
      Gtk::FILE_CHOOSER_ACTION_SAVE);
  if(consensus_filename.size())
    on_consensus_save1_activate();
}


void init_EdClustWindow(std::vector<clustering<std::string> >* clusterings){
  original_clusterings = _clusterings;
  pClusteringsList = Gtk::ListStore::create(model_Columns_clusterings);

  tvClusterings->set_model(pClusteringsList);
  tvClusterings->append_column("Clustering", model_Columns_clusterings.m_col_text);

  pItemList = Gtk::ListStore::create(model_Columns_items);
  tvItems->set_model(pItemList);
  tvItems->append_column("element", model_Columns_items.m_col_item);
  tvItems->append_column_numeric_editable("cluster", model_Columns_items.m_col_cluster_number, "%d");

  // update the clusterings when a cell has been edited
  Gtk::CellRendererText *renderer = (Gtk::CellRendererText*)tvItems->get_column_cell_renderer(1);
  renderer->signal_edited().connect(SigC::slot(*this, &wndEditClusterings::on_tvClusterings_renderer_edited) , true);

  tvClusterings->get_selection()->signal_changed().connect(
      SigC::slot(*this, &wndEditClusterings::on_tvClusterings_selection_changed) , false);

  // update the treeview and select the first clustering, if there are any
  update_tvClusterings();

  Gtk::TreeModel::iterator iter = pClusteringsList->children().begin();
  if(iter)
    tvClusterings->get_selection()->select(iter);
}

void on_edit1_activate()
{
  Gtk::ImageMenuItem* edit1;
  builder->get_widget("edit1", edit1);
  edit1->set_sensitive(false);

  Gtk::Window* EdClustWin;
  Glib::RefPtr<Gtk::Builder> EdClustBuilder = Gtk::Builder::create();
  try{
    EdClustBuilder->add_from_file("create_clustering.xml");
  } catch(const Glib::FileError& ex) {
    std::cerr << "FileError: " << ex.what() << std::endl;
    return;
  } catch(const Gtk::BuilderError& ex) {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    return;
  }
  EdClustBuilder->get_widget("wndEditClusterings", EdClustWin);

  gtk_builder_connect_signals( EdClustBuilder->gobj(), NULL );

  Gtk::Main::run(*EdClustWin);
  init_EdClustWindow(&clusterings);

  EditClusterings = new class wndEditClusterings(&clusterings);
  EditClusterings->signal_hide().connect(sigc::ptr_fun(&on_edit_complete), false);
}

void on_edit_complete()
{
  delete EditClusterings;
  edit1->set_sensitive(true);
  update_tvClusterings();
}

void on_open1_activate()
{
  clusterings_filename = select_a_file("please select a file to load clusterings from");

  if(clusterings_filename.size()){
    clusterings = read_clusterings_from_file<string>(clusterings_filename);

    // load the new clusterings into the treeview
    update_tvClusterings();
  }
}


void on_generate_randomly1_activate()
{
  srand(time(NULL));
  uint num_elements = (uint)(12*(double)rand()/RAND_MAX) + 3;
  uint num_clusterings = (uint)(16*(double)rand()/RAND_MAX) + 4;

  std::set<std::string> elements;
  std::stringstream s;
  for(uint i = 0; i < num_elements; i++){
    s.str(std::string());
    s << i;
    elements.insert(s.str());
  }

  clusterings.clear();
  for(uint i = 0; i < num_clusterings; i++)
    clusterings.push_back(generate_random_clustering(elements));

  update_tvClusterings();
}

void on_clusterings_save1_activate()
{
  if(clusterings_filename.size()){
    write_clusterings_to_file<std::string>(clusterings_filename, clusterings);
  } else on_clusterings_save_as1_activate();
}

void on_clusterings_save_as1_activate()
{
  clusterings_filename = select_a_file("please select a file to save the clusterings to",
      Gtk::FILE_CHOOSER_ACTION_SAVE);
  if(clusterings_filename.size())
    on_clusterings_save1_activate();
}



#endif // SIGNAL_HANDLER_HPP_INCLUDED
