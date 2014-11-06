
#include <sstream>
#include <iostream>
#include "edit_clusterings.hpp"

void edit_clusterings_window::get_my_elements(Glib::RefPtr<Gtk::Builder> builder){
  builder->get_widget("wndCreateClustering", window);

  builder->get_widget("btnNewClustering", btnNewClustering);
  builder->get_widget("btnDeleteClustering", btnDeleteClustering);
  builder->get_widget("btnNewItem", btnNewItem);
  builder->get_widget("btnDeleteItem", btnDeleteItem);
  builder->get_widget("btnOK", btnOK);
  builder->get_widget("btnCancel", btnCancel);

  builder->get_widget("tvClusterings", tvClusterings);
  builder->get_widget("tvItems", tvItems);
}

void edit_clusterings_window::connect_my_signals(){
  btnNewClustering->signal_clicked().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_btnNewClustering_clicked));
  btnDeleteClustering->signal_clicked().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_btnDeleteClustering_clicked));
  btnNewItem->signal_clicked().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_btnNewItem_clicked));
  btnDeleteItem->signal_clicked().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_btnDeleteItem_clicked));
  btnOK->signal_clicked().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_btnOK_clicked));
  btnCancel->signal_clicked().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_btnCancel_clicked));
}

edit_clusterings_window::edit_clusterings_window(vector<clustering<std::string> > *_clusterings):clusterings(*_clusterings){
  // create Gtk window using Gtk::Builder
  Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();

  try{
    builder->add_from_file(find_file("create_clustering.ui"));
  } catch(const Glib::FileError& ex) {
    std::cerr << "FileError: " << ex.what() << std::endl;
    exit(1);
  } catch(const Gtk::BuilderError& ex) {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    exit(1);
  }
  DEBUG("successfully build from create_clustering.ui"<<std::endl);

  get_my_elements(builder);
  DEBUG("got all elements"<<std::endl);
  connect_my_signals();
  DEBUG("connected all signals"<<std::endl);

  // content initialization
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
  renderer->signal_edited().connect(sigc::mem_fun(*this, &edit_clusterings_window::on_tvClusterings_renderer_edited) , true);

  tvClusterings->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &edit_clusterings_window::on_tvClusterings_selection_changed) , false);

  // update the treeview and select the first clustering, if there are any
  update_tvClusterings();

  Gtk::TreeModel::iterator iter = pClusteringsList->children().begin();
  if(iter)
    tvClusterings->get_selection()->select(iter);
}

vector<clustering<std::string> > edit_clusterings_window::get_clusterings() const{
  return clusterings;
}

void edit_clusterings_window::update_tvClusterings(const bool update_items){
  std::stringstream s;

  // save the selected row
  Gtk::TreeModel::iterator iter = tvClusterings->get_selection()->get_selected();
  Glib::ustring old_selection = "";
  if(iter){
    old_selection = pClusteringsList->get_string(iter);
    DEBUG("got clustering selection " << old_selection << std::endl);
  }
  // clear all rows
  pClusteringsList->clear();
  // redisplay all clusterings
  for(vector<clustering<std::string> >::iterator i = clusterings.begin();
      i != clusterings.end(); i++){
    // add row
    Gtk::TreeModel::Row row = *(pClusteringsList->append());

    // set values
    s.str(std::string());
    s << *i;
    row[model_Columns_clusterings.m_col_text] = s.str();
    row[model_Columns_clusterings.m_col_clustering] = &(*i);
  }
  DEBUG("done updating" << std::endl);
  if(old_selection.size()){
    DEBUG("setting clustering selection " << old_selection << std::endl);
    // restore the selected row
    tvClusterings->get_selection()->select(Gtk::TreePath(old_selection));

    // if the clusterings changed, then the consensus has to be recalculated
  }
  if(update_items) update_tvItems();
}

void edit_clusterings_window::update_tvItems(){
  std::stringstream s;

  // if a clustering was selected...
  Gtk::TreeModel::iterator tree_iter = tvClusterings->get_selection()->get_selected();
  if(tree_iter){
  	clustering<std::string> *currently_selected = tree_iter->get_value(model_Columns_clusterings.m_col_clustering);
    // ... which is not NULL
    if(currently_selected){
		  // save the selected row
		  Gtk::TreeModel::iterator iter = tvItems->get_selection()->get_selected();
		  Glib::ustring old_selection = "";
		  if(iter){
		    old_selection = pItemList->get_string(iter);
		    DEBUG("got item selection " << old_selection << std::endl);
		  }
      // clear all rows
      pItemList->clear();
    	// set values
    	for(clustering<std::string>::iterator i = currently_selected->begin();
    	    i != currently_selected->end(); i++){
    	  // add row
    	  Gtk::TreeModel::Row row = *(pItemList->append());

    	  // set values
    	  s.str(std::string());
    	  s << i->first;
    	  row[model_Columns_items.m_col_item] = s.str();
    	  row[model_Columns_items.m_col_cluster_number] = i->second;
    	  row[model_Columns_items.m_col_number_ptr] = &(i->second);
      }
      // restore the selected row
      if(old_selection.size()){
        DEBUG("setting item selection " << old_selection << std::endl);
        tvItems->get_selection()->select(Gtk::TreePath(old_selection));
      }
    }
  } else {
    // if no clustering is selected, check if there are clusterings at all
    // and if not, clear the ListStore
    if(!clusterings.size())
      pItemList->clear();
  }
}

#include <map>
void edit_clusterings_window::on_tvClusterings_renderer_edited(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
  Gtk::TreePath path(path_string);
  Gtk::TreeModel::iterator i = pItemList->get_iter(path);
  uint* cluster_number = i->get_value(model_Columns_items.m_col_number_ptr);

  std::istringstream read_number(new_text);
  if(cluster_number) read_number >> *cluster_number;

  // reassign cluster numbers such that there are no empty clusters
  std::map<uint, uint> cluster_relation;
  // unclustered elements stay unclustered
  cluster_relation.insert(pair<uint, uint>(0,0));

  uint new_cluster_number = 1;
  Gtk::TreeModel::Children rows = pItemList->children();
  for(Gtk::TreeModel::Children::iterator i = rows.begin(); i != rows.end(); i++){
    cluster_number = i->get_value(model_Columns_items.m_col_number_ptr);
    if(*cluster_number)
      if(cluster_relation.find(*cluster_number) == cluster_relation.end())
        cluster_relation.insert(pair<uint,uint>(*cluster_number, new_cluster_number++));
  }
  // change all cluster numbers to the newly calculated ones
  for(Gtk::TreeModel::Children::iterator i = rows.begin(); i != rows.end(); i++){
    cluster_number = i->get_value(model_Columns_items.m_col_number_ptr);
    *cluster_number = cluster_relation[*cluster_number];
  }

  update_tvClusterings();
}

void edit_clusterings_window::on_tvClusterings_selection_changed()
{
  // is it a deselection?
  Gtk::TreeModel::iterator tree_iter = tvClusterings->get_selection()->get_selected();
  // if not, update the Items-treeview
  if(tree_iter)
    update_tvItems();
}

void edit_clusterings_window::on_btnNewClustering_clicked()
{
  clustering<std::string> new_clust;
  clustering<std::string> first_clustering;

  if(clusterings.begin() != clusterings.end()){
    first_clustering = *(clusterings.begin());
    if(first_clustering.begin() != first_clustering.end()){
      for(clustering<std::string>::const_iterator i = first_clustering.begin(); i != first_clustering.end(); i++)
        new_clust.insert(pair<std::string,uint>(i->first, 1));
    }
    clusterings.push_back(new_clust);
    update_tvClusterings();
  }
}

void edit_clusterings_window::on_btnDeleteClustering_clicked()
{
  Gtk::TreeModel::iterator tree_iter = tvClusterings->get_selection()->get_selected();
  // if nothing is selected, do not delete anything
  if(tree_iter){
  	clustering<std::string> *currently_selected = tree_iter->get_value(model_Columns_clusterings.m_col_clustering);
    for(vector<clustering<std::string> >::iterator i = clusterings.begin(); i != clusterings.end(); i++)
      if(&(*i) == currently_selected) {
        clusterings.erase(i);
        break;
      }
    update_tvClusterings();
  }
}

void edit_clusterings_window::on_btnNewItem_clicked()
{
  // additionally, create a new clustering if there are none
  if(!clusterings.size())
    clusterings.push_back(clustering<std::string>());
  ostringstream item_name("1");
  uint temp = 1;
  // find an item name that is not already taken
  // TODO: promt user input here
  while(clusterings.begin()->find(item_name.str()) != clusterings.begin()->end()){
    item_name.str(std::string());
    item_name << ++temp;
  }
  // create a new cluster in each clustering containing just the new item
  for(vector<clustering<std::string> >::iterator i = clusterings.begin(); i != clusterings.end(); i++)
    i->insert(pair<std::string,uint>(item_name.str(), num_clusters(*i) + 1));

  update_tvClusterings();
}

void edit_clusterings_window::on_btnDeleteItem_clicked()
{
  // get selected item
  Gtk::TreeModel::iterator iter = tvItems->get_selection()->get_selected();
	if(iter){
  	std::string currently_selected = iter->get_value(model_Columns_items.m_col_item);

    // delete the selected item from all clusterings
    for(vector<clustering<std::string> >::iterator i = clusterings.begin(); i != clusterings.end(); i++)
      i->erase(currently_selected);

    update_tvClusterings();
  }
}

void edit_clusterings_window::on_btnOK_clicked()
{
  *original_clusterings = clusterings;
  window->hide();
}

void edit_clusterings_window::on_btnCancel_clicked()
{
  window->hide();
}
