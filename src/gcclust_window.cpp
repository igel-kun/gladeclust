
#include "globals.hpp"
#include <iostream>
#include <sstream>
#include <set>
#include <time.h>
#include "gcclust_window.hpp"


void gcclust_window::connect_my_signals(){
  about1->signal_activate().connect(sigc::mem_fun(*this, &gcclust_window::on_about1_activate));
  quit1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_quit1_activate));
  new1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_new1_activate));
  edit1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_edit1_activate));
  open1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_open1_activate));
  generate_randomly1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_generate_randomly1_activate));
  clusterings_save1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_clusterings_save1_activate));
  clusterings_save_as1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_clusterings_save_as1_activate));
  consensus_save1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_consensus_save1_activate));
  consensus_save_as1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_consensus_save_as1_activate));
  preprocess_clusterings1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_preprocess_clusterings1_activate));
  brute_force_search1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_brute_force_search1_activate));
  measure_time1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_measure_time1_activate));
  compute_consensus1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_compute_consensus1_activate));
  cancel1->signal_activate().connect(sigc::mem_fun(*this,&gcclust_window::on_cancel1_activate));
}

void gcclust_window::get_my_elements(Glib::RefPtr<Gtk::Builder> builder){
  builder->get_widget("gcclust_window", window);

  builder->get_widget("lblClusteringsFrame", lblClusteringsFrame);
  builder->get_widget("lblConsensusFrame", lblConsensusFrame);
  builder->get_widget("lblProgress", lblProgress);
  builder->get_widget("pgbProgress", pgbProgress);
  builder->get_widget("preprocess_clusterings1", preprocess_clusterings1);
  builder->get_widget("apply_exhaustively1", apply_exhaustively1);
  builder->get_widget("measure_time1", measure_time1);
  builder->get_widget("cancel1", cancel1);
  builder->get_widget("tvClusterings", tvClusterings);
  builder->get_widget("tvConsensus", tvConsensus);

  builder->get_widget("about1", about1);
  builder->get_widget("quit1", quit1);
  builder->get_widget("new1", new1);
  builder->get_widget("edit1", edit1);
  builder->get_widget("open1", open1);
  builder->get_widget("generate_randomly1", generate_randomly1);
  builder->get_widget("clusterings_save1", clusterings_save1);
  builder->get_widget("clusterings_save_as1", clusterings_save_as1);
  builder->get_widget("consensus_save1", consensus_save1);
  builder->get_widget("consensus_save_as1", consensus_save_as1);
  builder->get_widget("brute_force_search1", brute_force_search1);
  builder->get_widget("compute_consensus1", compute_consensus1);
}

// default values: 100us timer
gcclust_window::gcclust_window() : timer_thread(100, &signal_progress_pc){

  // create Gtk window using Gtk::Builder
  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();

  try{
    builder->add_from_file(find_file("gcclust.ui"));
  } catch(const Glib::FileError& ex) {
    std::cerr << "FileError: " << ex.what() << std::endl;
    exit(1);
  } catch(const Gtk::BuilderError& ex) {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    exit(1);
  }

  DEBUG("successfully build from gcclust.ui"<<std::endl);

  get_my_elements(builder);
  DEBUG("got all elements"<<std::endl);
  connect_my_signals();
  DEBUG("connected all signals"<<std::endl);

  // content initialization

  // define model and add view columns for both treeviews
  pClusteringsList = Gtk::ListStore::create(model_Columns_clusterings);
  tvClusterings->set_model(pClusteringsList);
  tvClusterings->append_column("Clustering", model_Columns_clusterings.m_col_text);

  pConsensusList = Gtk::ListStore::create(model_Columns_clusterings);
  tvConsensus->set_model(pConsensusList);
  tvConsensus->append_column("Consensus", model_Columns_clusterings.m_col_text);

  lblClusteringsFrame->set_label("input Clusterings (average distance: ...)");

  // connect the progress bar update routine to the appropriate dispatcher
  signal_progress_pc.connect(sigc::mem_fun(*this, &gcclust_window::update_percent));

  // time measurement
  measure_time = measure_time1->get_active();
  // threading
  preprocess_thread = NULL;
  searchtree_thread = NULL;
  // misc stuff
  cancel1->set_sensitive(false);
  // this will automtically update the tvConsensus as well
  update_tvClusterings();
}

gcclust_window::~gcclust_window(){
  if(preprocess_thread) delete preprocess_thread;
  if(searchtree_thread) delete searchtree_thread;
  // TODO: delete the builder
}

std::string gcclust_window::select_a_file(const std::string caption, const Gtk::FileChooserAction action){
	Gtk::FileChooserDialog dialog(caption, action);
	dialog.set_transient_for(*window);

	//Add response buttons the the dialog:
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  Gtk::FileFilter filter_any;
	filter_any.set_name("Any files");
	filter_any.add_pattern("*");
	dialog.add_filter(filter_any);

  Gtk::FileFilter filter_text;
  filter_text.set_name("Text files");
  filter_text.add_mime_type("text/plain");
  dialog.add_filter(filter_text);

  // Show the dialog and wait for a user response:
  int result = dialog.run();

	// Handle the response:
  std::string filename;
	switch(result){
  	case(Gtk::RESPONSE_OK):{
        filename = dialog.get_filename();
    	break;
    }
  	case(Gtk::RESPONSE_CANCEL):{
      break;
    }
    default:{
      break;
    }
	}
  return filename;

}

// when idle, calculate the average distance of the input clusterings,
// if they are less then 200 clusterings and less than 200 elements
bool gcclust_window::calc_avg_dist_when_idle(){
  if(clusterings.size() && (clusterings.size()<200) && (clusterings.begin()->size()<200)){
    DEBUG("we are idle, lets calc avg distances (" << clusterings.size() << " clusterings, " << clusterings.begin()->size() << "elements)..." << std::endl);
    stringstream s;
    s.precision(4);
    s << "input Clusterings (average distance: " << get_avg_distance(clusterings) << ")";
    lblClusteringsFrame->set_label(s.str());
    // remove the signal handler
  }
  return false;
}

void gcclust_window::update_tvClusterings(){
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
  Glib::signal_idle().connect(sigc::mem_fun(*this, &gcclust_window::calc_avg_dist_when_idle) );
}

void gcclust_window::update_tvConsensus(){
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

// the user pressed the 'compute consensus' button
void gcclust_window::on_compute_consensus1_activate()
{
  lblProgress->set_label("preprocess:");

  uint preprocessing = 0;
  if(preprocess_clusterings1->get_active())
    preprocessing = (apply_exhaustively1->get_active()?(uint)(-1):1);

  DEBUG("preprocessing " << preprocessing << " times" << std::endl);

  comp_done_con.disconnect();
  comp_done_con = signal_computation_done.connect(
      sigc::mem_fun(*this, &gcclust_window::preprocess_complete));

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
void gcclust_window::preprocess_complete(){
  DEBUG("preprocessing complete" << std::endl);
  cancel1->set_sensitive(false);
  cancel_computation = false;

  // if the 'brute force' option is selected, start the searchtree_thread
  if(brute_force_search1->get_active()){
    lblProgress->set_label("brute force:");

	  comp_done_con.disconnect();
	  comp_done_con = signal_computation_done.connect(
	      sigc::mem_fun(*this, &gcclust_window::searchtree_complete));

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
void gcclust_window::searchtree_complete(){
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

// this function is called every 100us by the timer thread
void gcclust_window::update_percent(){
  stringstream s;
  s.precision(4);
  s << 100*progress_pc << "\%";
  pgbProgress->set_text(s.str());
  pgbProgress->set_fraction(progress_pc);
}

// callback function for the search tree algorithm to signal that its
// computation started
void gcclust_window::brute_start(const clustering<std::string> &cons){
  consensus = cons;
  update_tvConsensus();
  lblProgress->set_label("search tree:");
}

// *************************************
// ********** boring stuff *************
// *************************************

void gcclust_window::on_measure_time1_activate()
{
  measure_time = measure_time1->get_active();
}

void gcclust_window::on_preprocess_clusterings1_activate()
{
}

void gcclust_window::on_brute_force_search1_activate()
{
}


void gcclust_window::on_quit1_activate()
{
  Gtk::Main::quit();
}

void gcclust_window::on_new1_activate()
{
  clusterings.clear();
  update_tvClusterings();
}


void gcclust_window::on_cancel1_activate()
{
  cancel_computation = true;
}

void gcclust_window::on_consensus_save1_activate()
{
  if(consensus_filename.size()){
    write_clustering_to_file<std::string>(consensus_filename, consensus);
  } else on_consensus_save_as1_activate();
}

void gcclust_window::on_consensus_save_as1_activate()
{
  consensus_filename = select_a_file("please select a file to save the consensus clustering to",
      Gtk::FILE_CHOOSER_ACTION_SAVE);
  if(consensus_filename.size())
    on_consensus_save1_activate();
}

void gcclust_window::on_edit1_activate()
{
  edit1->set_sensitive(false);
  EditClusterings = new class edit_clusterings_window(&clusterings);
  // add callback to when the edit window is closed
  EditClusterings->window->signal_hide().connect(sigc::mem_fun(*this, &gcclust_window::on_edit_complete), false);
}

void gcclust_window::on_edit_complete()
{
  delete EditClusterings;
  edit1->set_sensitive(true);
  update_tvClusterings();
}

void gcclust_window::on_open1_activate()
{
  clusterings_filename = select_a_file("please select a file to load clusterings from");

  if(clusterings_filename.size()){
    clusterings = read_clusterings_from_file<string>(clusterings_filename);

    // load the new clusterings into the treeview
    update_tvClusterings();
  }
}


void gcclust_window::on_generate_randomly1_activate()
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

void gcclust_window::on_clusterings_save1_activate()
{
  if(clusterings_filename.size()){
    write_clusterings_to_file<std::string>(clusterings_filename, clusterings);
  } else on_clusterings_save_as1_activate();
}

void gcclust_window::on_clusterings_save_as1_activate()
{
  clusterings_filename = select_a_file("please select a file to save the clusterings to",
      Gtk::FILE_CHOOSER_ACTION_SAVE);
  if(clusterings_filename.size())
    on_clusterings_save1_activate();
}
void gcclust_window::on_about1_activate()
{
	Gtk::AboutDialog dialog;
	dialog.set_transient_for(*window);

  std::vector<std::string> authors;
  std::vector<std::string> documenters = std::vector<std::string>();
  std::vector<std::string> artists = std::vector<std::string>();
//  std::string licence = "GPL2";
  std::string copyright = "(c) 2009 Mathias Weller";
  std::string name = "gcclust - gnome consensus clustering";
  std::string version = "0.1";
  std::string website = "http://www.minet.uni-jena.de/~igel/cclust";
  std::string website_label = "homepage";
  std::string comments = "Preprocessing Algorithm follows an approach by\n\
                          Nadja Betzler <nadja.betzler@uni-jena.de>\n\
                          Jiong Guo <jiong.guo@uni-jena.de>\n\
                          Christian Komusiewicz <ckomus@uni-jena.de>\n\
                          Rolf Niedermeier <rolf.niedermeier@uni-jena.de>\n\
                          \"Average Parameterization for Computing Medians\"";

  authors.push_back("Mathias Weller <mathias.weller@uni-jena.de>");

  dialog.set_artists(artists);
  dialog.set_authors(authors);
  dialog.set_comments(comments);
  dialog.set_documenters(documenters);
//  dialog.set_license(licence);
  dialog.set_copyright(copyright);
  dialog.set_name(name);
  dialog.set_version(version);
  dialog.set_website(website);
  dialog.set_website_label(website_label);

  dialog.run();
}


