#ifndef EDIT_CLUSTERINGS_HH
#define EDIT_CLUSTERINGS_HH

#include "globals.hpp"
#include <gtkmm.h>
#include "cclust_pthread.h"


class edit_clusterings_window{
  // treeview stuff
  class ClusteringColumns : public Gtk::TreeModel::ColumnRecord{
    // TODO: add a "distance" column
  	public:
  	ClusteringColumns(){ add(m_col_text); add(m_col_clustering);}
  	Gtk::TreeModelColumn<std::string> m_col_text;
    // this is hidden data, because it is not being added as a view column
  	Gtk::TreeModelColumn<clustering<std::string>* > m_col_clustering;
  };
  ClusteringColumns model_Columns_clusterings;
  Glib::RefPtr<Gtk::ListStore> pClusteringsList;

  class ItemColumns : public Gtk::TreeModel::ColumnRecord{
    // TODO: add a "distance" column
  	public:
  	ItemColumns(){ add(m_col_item); add(m_col_cluster_number); add(m_col_number_ptr);}
  	Gtk::TreeModelColumn<std::string> m_col_item;
  	Gtk::TreeModelColumn<uint> m_col_cluster_number;
  	Gtk::TreeModelColumn<uint*> m_col_number_ptr;
  };
  ItemColumns model_Columns_items;
  Glib::RefPtr<Gtk::ListStore> pItemList;

  vector<clustering<std::string> > clusterings;
  vector<clustering<std::string> > *original_clusterings;
  vector<clustering<std::string> > get_clusterings() const;

  void update_tvClusterings(const bool update_items=true);
  void update_tvItems();


  // ========== window elements we use ==================
  public:
    Gtk::Window*        window;
  private:
    Gtk::Button*        btnNewClustering;
    Gtk::Button*        btnDeleteClustering;
    Gtk::Button*        btnNewItem;
    Gtk::Button*        btnDeleteItem;
    Gtk::Button*        btnOK;
    Gtk::Button*        btnCancel;
    Gtk::TreeView*      tvClusterings;
    Gtk::TreeView*      tvItems;

  private:
    void get_my_elements(Glib::RefPtr<Gtk::Builder> builder);
    void connect_my_signals();

  // ============ signal handlers ====================
    void on_tvClusterings_renderer_edited(const Glib::ustring& path_string, const Glib::ustring& new_text);
    void on_tvClusterings_selection_changed();
    void on_btnNewClustering_clicked();
    void on_btnDeleteClustering_clicked();
    void on_btnNewItem_clicked();
    void on_btnDeleteItem_clicked();
    void on_btnOK_clicked();
    void on_btnCancel_clicked();


  public:
    // constructors & destructors
    edit_clusterings_window(std::vector<clustering<std::string> >* _clustering);

};

#endif // EDIT_CLUSTERINGS_HH
