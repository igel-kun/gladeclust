/* This is cclust.h - an implementation of an exact method to solving
 * the CONSENSUS CLUSTERING problem
 *
 * use the get_consensus_clustering() function to calculate a consensus
 * clustering for a given vector of clusterings, note that this function
 * can be specified to either preprocess the input of not
 * the preprocessing routine follows an approach by
 * Nadja Betzler, Christian Komusiewicz, Jiong Guo, and Rolf Niedermeier
 * of the university of jena/germany
 *
 * author of this file is Mathias Weller <mathias.weller@uni-jena.de>
 */

#ifndef cclust_h
#define cclust_h

#include <string>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <math.h>
#include <cstdlib>

using namespace std;

enum relation{
  REL_PRED_COED,    // predominantly co-clusteringed ( >2/3 co )
  REL_PRED_ANTIED,  // predominantly anti-clusteringed ( <1/3 co )
  REL_DIRTY,        // dirty (1/3 <= co <= 2/3)
  REL_UNKNOWN
};

// clustering<T> is an alias for map<T,uint>
template <typename T>
class clustering: public map<T,uint>{};

// for each item 'item' there are special properties over all given clusterings
template <typename T>
class iteminfo {
  public:
  set<T> pred_coed_with;  // the items that 'item' is predominantly co-clusteringed with
  set<T> pred_antied_with;// the items that 'item' is predominantly anti-clusteringed with
  set<T> dirty_with;      // the items that form a dirty pair with 'item'
  bool accounted_for;
};


// ************************************************************************
// ************************** function definitions ************************
// ************************************************************************

// return the number of clusters in the clustering C
template <typename T>
uint num_clusters(const clustering<T>& C){
  set<uint> clusters;
  if(C.empty()) return 0; else{
  	for(typename clustering<T>::const_iterator i = C.begin(); i != C.end(); i++)
      if(i->second) clusters.insert(i->second); // 0 is a special cluster for 'unclustered yet'
    return clusters.size();
  }
}

// add a cluster to the clustering C
template <typename T>
void add_cluster(clustering<T>& C, const set<T>& cluster){
  uint cluster_num = num_clusters(C) + 1;
  for(typename set<T>::const_iterator x = cluster.begin(); x != cluster.end(); x++)
    C[*x] = cluster_num;
}

// remove an element from a clustering C
template <typename T>
inline void remove_element(clustering<T>& C, const T& element){
  C.erase(element);
}
// remove an element from a vector of clusterings
template <typename T>
inline void remove_element(vector<clustering<T> >& clusterings, const T& element){
  for(typename vector<clustering<T> >::iterator C = clusterings.begin();
      C != clusterings.end(); C++)
    remove_element(*C, element);
}
// remove a set of elements from a clustering C
template <typename T>
inline void remove_elements(clustering<T>& C, const set<T>& elements){
  for(typename set<T>::const_iterator x = elements.begin(); x != elements.end(); x++)
    remove_element(C, *x);
}
// remove a set of elements from a vector of clusterings
template <typename T>
inline void remove_elements(vector<clustering<T> >& clusterings, const set<T>& elements){
  for(typename set<T>::const_iterator x = elements.begin(); x != elements.end(); x++)
    remove_element(clusterings, *x);
}

// return whether a and b are co-clustered in C
template <typename T>
inline bool coed(const T& a, const T& b, const clustering<T>& C){
  return C.find(a)->second == C.find(b)->second;
}

// return whether a and b are anti-clustered in C
template <typename T>
inline bool antied(const T& a, const T& b, const clustering<T>& C){
  return C[a] != C[b];
}

// determine the unclustered elements
template <typename T>
inline set<T> get_clustered_elements(const clustering<T>& C){
  set<T> result;
  for(typename clustering<T>::const_iterator i = C.begin(); i != C.end(); i++)
    if(i->second) result.insert(i->first);
  return result;
}
// determine the unclustered elements
template <typename T>
inline set<T> get_unclustered_elements(const clustering<T>& C){
  set<T> result;
  for(typename clustering<T>::const_iterator i = C.begin(); i != C.end(); i++)
    if(!i->second) result.insert(i->first);
  return result;
}

// calculate the distance between two clusterings
template <typename T>
uint get_distance(const clustering<T>& C1, const clustering<T>& C2){
	uint disagree = 0;
	// calc for how many unordered pairs the clusterings C1 and C2 disagree
	for(typename clustering<T>::const_iterator i = C1.begin(); i != C1.end();	i++)
		for(typename clustering<T>::const_iterator j = i; j != C1.end(); j++) if(i != j)
			if(coed(i->first, j->first, C1) != coed(i->first, j->first, C2))
				disagree++;
	return disagree;
}

// calculate the accumulated distances between a clustering and a vector of clusterings
template <typename T>
uint get_distance(const clustering<T>& C, const vector<clustering<T> >& clusterings){
  uint dist = 0;
  for(typename vector<clustering<T> >::const_iterator j = clusterings.begin();
      j != clusterings.end(); j++)
    dist += get_distance(C, *j);
  return dist;
}

// calculate the average distance of a vector of clusterings
template <typename T>
double get_avg_distance(const vector<clustering<T> >& clusterings){
	uint accu = 0;
	// sum up the distance of every pair of clusterings
	// exploit that d(C,C) == 0  and  d(C1,C2) == d(C2,C1)
	for(typename vector<clustering<T> >::const_iterator i = clusterings.begin(); i != clusterings.end(); i++)
		for(typename vector<clustering<T> >::const_iterator j = i; j != clusterings.end(); j++) if(i != j)
			accu += get_distance(*i,*j);
	// so far we calculated 1/2 of the accumulated distance
	return ((double)(accu<<1))/((double)clusterings.size());
}

// merge two disjoint (!) clusterings into one
// if the merge_unclustered flag is set, then merge the unclustered items of both clusterings
// if the merge_unclustered flag is not set, then discard any unclustered items
template <typename T>
clustering<T> merge_clusterings(const clustering<T>& C1, const clustering<T> &C2, const bool merge_unclustered = true){
  map<uint, set<T> > clusters = map<uint, set<T> >();
  clustering<T> result;

  // first, copy all clusters from C1 (except unclustered, if merge_unclustered is not set)
  for(typename clustering<T>::const_iterator i = C1.begin(); i != C1.end(); i++)
    if((i->second) || (merge_unclustered)) result.insert(*i);
  // second, make a list of clusters from C2 to merge into result
  for(typename clustering<T>::const_iterator i = C2.begin(); i != C2.end(); i++){
    if(clusters.find(i->second) == clusters.end())
      clusters.insert(pair<uint,set<T> >(i->second,set<T>()));
    clusters[i->second].insert(i->first);
  }
  // third, merge the clusters in the list into result
  for(typename map<uint, set<T> >::const_iterator i = clusters.begin(); i != clusters.end(); i++)
    if(i->first > 0)
      add_cluster(result, i->second);
    else
      if(merge_unclustered)
        for(typename set<T>::const_iterator j = i->second.begin(); j != i->second.end(); j++)
          if(result.find(*j) == result.end()) result.insert(pair<T,uint>(*j,0));
  return result;
}


// return whether two elements are predominently coed, antied or dirty
template <typename T>
relation get_relation(const iteminfo<T>& info, const T& b){
  if(info.pred_coed_with.find(b) != info.pred_coed_with.end()) return REL_PRED_COED;
  if(info.pred_antied_with.find(b) != info.pred_antied_with.end()) return REL_PRED_ANTIED;
  if(info.dirty_with.find(b) != info.dirty_with.end()) return REL_DIRTY;
  return REL_UNKNOWN;
}

// return whether two elements are predominently coed, antied or dirty
template <typename T>
relation get_relation(const T& a, const T& b, const map<T,iteminfo<T> >& infos){
  // keep in mind that the order of which a and b are given matters !
  // please make sure that a < b
  return get_relation(infos[a],b);
}

// set two elements predominently coed, antied or dirty
template <typename T>
void set_relation(iteminfo<T> *info, const T& b, const relation r){
  switch(r){
    case REL_PRED_COED:
      info->pred_coed_with.insert(b);
      break;
    case REL_PRED_ANTIED:
      info->pred_antied_with.insert(b);
      break;
    case REL_DIRTY:
      info->dirty_with.insert(b);
      break;
    case REL_UNKNOWN:
      info->pred_coed_with.erase(b);
      info->pred_antied_with.erase(b);
      info->dirty_with.erase(b);
      break;
  }
}

// set two elements predominently coed, antied or dirty
template <typename T>
void set_relation(const T& a, const T& b, map<T,iteminfo<T> >& infos, const relation r){
  // keep in mind that the order of which a and b are given matters !
  // please make sure that a < b
  set_relation(&infos[a],b,r);
}

// compute the number of dirty pairs related to the eq-class
// this can be done by summing up |dirty_with[x]| for all x in dirty_part
template <typename T>
uint num_dirty_pairs(const set<T>& elements, const map<T,iteminfo<T> >& infos){
   uint dirty_pairs = 0;
   typename map<T,iteminfo<T> >::const_iterator info_iterator;
   for(typename set<T>::const_iterator x = elements.begin(); x != elements.end(); x++)
     if((info_iterator = infos.find(*x)) != infos.end())
       dirty_pairs += info_iterator->second.dirty_with.size();
   return dirty_pairs;
}


// apply the preprocessing Rule 1 [see the paper mentioned above] exhaustively
// and return a partial solution
// we assume all clusterings to be over the same set of elements
template <typename T>
clustering<T> apply_preprocessing(const vector<clustering<T> >& clusterings,
    const clustering<T>& partial_clustering = clustering<T>(),
    double* progress_pc = NULL,
    const bool* cancel_computation = NULL){
  set<T> unclustered;
  set<T> global_dirty;    // set of all dirty items
  map<T,iteminfo<T> >  infos;

  // first, make a non-constant copy of the partial clustering
  clustering<T> optimal_clustering(partial_clustering);
  // if the current_clustering is new, set all items to unclustered
  if(optimal_clustering == clustering<T>()){
    if(clusterings.size()){
    for(typename clustering<T>::const_iterator i = clusterings.begin()->begin();
      i != clusterings.begin()->end(); i++)
      optimal_clustering.insert(pair<T,uint>(i->first,0));
    } else return optimal_clustering;
  }

  unclustered = get_unclustered_elements(optimal_clustering);

  if(unclustered.size()){
	  // calculate the number of steps that will be taken
	  uint all_steps = unclustered.size();
	  all_steps = ((all_steps + 1) * all_steps) >> 1;
	  // so far, 0 steps have been taken
	  uint steps = 0;

	  // for each element, compute its infos, that is, the sets of elements that are
	  // predominantly co-clustered, anti-clustered, or form a dirty pair with it
	  uint count_coed;
	  for(typename set<T>::const_iterator i = unclustered.begin(); i != unclustered.end(); i++){
	    infos[*i].pred_coed_with.insert(*i);
	//    info = &infos[i->first];
	    for(typename set<T>::const_iterator j = i; j != unclustered.end(); j++) if(i != j){
	      if(progress_pc) *progress_pc = ((double)steps)/all_steps;
	      if(cancel_computation)
	        if(*cancel_computation) return clustering<T>();
	      // compute the relation by iterating over the clusterings
	      count_coed = 0;
		    for(typename vector<clustering<T> >::const_iterator C = clusterings.begin();
	          C != clusterings.end(); C++)
	        if(coed(*i, *j, *C)) count_coed++;
	      // check whether (i,j) are predominantly coed, antied or dirty
	      if(count_coed < ((double)clusterings.size())/3){
	        infos[*i].pred_antied_with.insert(*j);  // predominantly antied
	        infos[*j].pred_antied_with.insert(*i);  // predominantly antied
	      } else {
	        if(count_coed > ((double)(clusterings.size()*2))/3){
	          infos[*i].pred_coed_with.insert(*j);  // predominantly coed
	          infos[*j].pred_coed_with.insert(*i);  // predominantly coed
	        } else {
	          infos[*i].dirty_with.insert(*j);      // neither predominently coed, nor antied
	          infos[*j].dirty_with.insert(*i);      // neither predominently coed, nor antied
	          global_dirty.insert(*i);
	          global_dirty.insert(*j);
	        }
	      }
	      steps++;
	    }
	  }


	  // now, walk through the elements and construct their equivalence classes
	  for(typename set<T>::const_iterator i = unclustered.begin(); i != unclustered.end(); i++){
	    infos[*i].accounted_for = false;
	//    optimal_clustering[*i] = 0; // 0 = not clustered yet
	  }
	  set<T> clean_part, dirty_part, equiv_class, dirty_equiv_class;
	  for(typename set<T>::const_iterator i = unclustered.begin(); i != unclustered.end(); i++){
	    if(progress_pc) *progress_pc = ((double)steps)/all_steps;
	    if(cancel_computation)
	      if(*cancel_computation) return clustering<T>();

	    clean_part.clear();
	    dirty_part.clear();
	    // get the first element in the list that is not dirty or already accounted for
	    if((!infos[*i].accounted_for) && (global_dirty.find(*i) == global_dirty.end())){
		      // its equivalence class is the set of all co-clustered elements
		      equiv_class = infos[*i].pred_coed_with;

	        // we split those elements in dirty and clean ones by intersecting with global_dirty
		      set_intersection(equiv_class.begin(), equiv_class.end(),
		            global_dirty.begin(), global_dirty.end(), inserter(dirty_part, dirty_part.begin()));
		      set_difference(equiv_class.begin(), equiv_class.end(),
		            dirty_part.begin(), dirty_part.end(), inserter(clean_part, clean_part.begin()));

	        // if now the non-dirty part is larger than the dirty pairs,
	        // then the eq-class is part of the optimal clustering
	        if(clean_part.size() > num_dirty_pairs(dirty_part, infos)){
		        // add the eq-class as a cluster to optimal_clustering
		        add_cluster(optimal_clustering, equiv_class);
		        // remove the elements in the eq-class from all clusterings
	 //         remove_elements(clusterings, equiv_class);
		        // mark the members of the eq-class as accounted for
	          for(typename set<T>::const_iterator x = equiv_class.begin(); x != equiv_class.end(); x++)
		          infos[*x].accounted_for = true;
	        }
	    }
	    steps++;
	  }
	  if(progress_pc) *progress_pc = ((double)steps)/all_steps;
  }
  return optimal_clustering;
}

// complete (assign clusters to all unclustered elements) the given optimal
// clustering by performing a brute force search on the clusterings getting
// the optimal consensus clustering for the given instance
template <typename T>
clustering<T> get_consensus_clustering_brute(const vector<clustering<T> >& clusterings,
                                            clustering<T> current_clustering = clustering<T>(),
                                            double *progress_pc = NULL,
                                            const bool* cancel_computation = NULL,
                                            double current_pc = 0,
                                            double max_pc = 1)
{
  if(cancel_computation)
      if(*cancel_computation) return clustering<T>();
  // if the current_clustering is new, set all items to unclustered
  if(current_clustering == clustering<T>()){
    if(clusterings.size()){
      for(typename clustering<T>::const_iterator i = clusterings.begin()->begin();
       i != clusterings.begin()->end(); i++)
        current_clustering.insert(pair<T,uint>(i->first,0));
    } else return current_clustering;
  }
  // find the first unclustered element
  T *first_unclustered = NULL;
  for(typename clustering<T>::const_iterator i = current_clustering.begin();
      i != current_clustering.end(); i++)
    if(!i->second) { first_unclustered = new T(i->first); break;}
  // if all elemtents are clustered, return the current clustering...
  if(!first_unclustered) return current_clustering;

  // ... else, try to put it into clusters 1...num_clusters+1 and branch
  clustering<T> best_clustering;
  uint max_cluster = num_clusters(current_clustering) + 1;
  uint dist = 0;
  uint min_distance = (uint)-1;
  clustering<T> min_clustering;
  for(uint cluster = 1; cluster <= max_cluster; cluster++){
    current_clustering[*first_unclustered] = cluster;
    best_clustering = get_consensus_clustering_brute(clusterings, current_clustering,
        progress_pc, cancel_computation,
        current_pc , current_pc + (max_pc - current_pc)/max_cluster);
    current_pc += (max_pc - current_pc)/max_cluster;

    // user do not notice any increase below 1%
    if(max_pc - current_pc > 0.01)
      if(progress_pc) *progress_pc = current_pc;

    // accumulated distance to all clusterings
    dist = get_distance(best_clustering, clusterings);
    if(dist < min_distance){
      min_distance = dist;
      min_clustering = best_clustering;
    }
    if(min_distance == 0) break;
  }
  if(progress_pc) *progress_pc = max_pc;
  delete first_unclustered;
  return min_clustering;
}


// calculate a clustering with minimum sum of distances to all given clusterings
// we assume all clusterings to be over the same set of elements
template <typename T>
clustering<T> get_consensus_clustering(vector<clustering<T> >& clusterings,
                                      const bool do_preprocessing = true,
                                      double* preprocessing_percent = NULL,
                                      double* brute_force_percent = NULL){
  clustering<T> optimal_clustering;
  uint old_clustered;
  uint new_clustered = 0;
  // apply preprocessing
  if(do_preprocessing) do{
      old_clustered = new_clustered;
      optimal_clustering = apply_preprocessing(clusterings, optimal_clustering);
      new_clustered = get_clustered_elements(optimal_clustering).size();
    } while(old_clustered < new_clustered);
  // and search with brute force on the remaining instance
  optimal_clustering = get_consensus_clustering_brute(clusterings, optimal_clustering);
  return optimal_clustering;
}

// the following function is mostly copy-past from
// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
inline vector<string> tokenize(const string& str, const string& delimiters = " "){
  vector<string> tokens;
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(delimiters, lastPos);
  while (string::npos != pos || string::npos != lastPos){
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
  return tokens;
}

// read a vector of clusterings from a stream
template <typename T>
vector<clustering<T> > read_clusterings(istream& is){
  uint num_clusterings;
  uint len_clusterings;
  vector<clustering<T> > clusterings;
  clustering<T> T_clustering;
  set<T> current_cluster;
  vector<string> str_clustering;
  vector<string> str_clusterings;
  string name, elements;

  is >> len_clusterings >> num_clusterings >> name;
  for(uint i = 0; i < len_clusterings; i++) getline(is, name);
  for(uint i = 0; i < num_clusterings; i++){
    getline(is, name);
    getline(is, elements,'.');
    getline(is, name);
    T_clustering = clustering<T>();
    str_clusterings = tokenize(elements, ";");
    for(vector<string>::const_iterator j = str_clusterings.begin(); j != str_clusterings.end(); j++){
      str_clustering = tokenize(*j, ",");
      current_cluster = set<T>();
      for(vector<string>::const_iterator k = str_clustering.begin(); k != str_clustering.end(); k++)
        current_cluster.insert((T)(*k));
      add_cluster(T_clustering, current_cluster);
    }
    clusterings.push_back(T_clustering);
  }
  return clusterings;
}

// write a vector of clusterings to a stream
template <typename T>
void write_clusterings(ostream& os, const vector<clustering<T> > clusterings){
  uint num_clusterings = clusterings.size();
  uint len_clusterings = clusterings[0].size();

  os << len_clusterings << " " << num_clusterings << "\n";
  for(typename clustering<T>::const_iterator i = clusterings[0].begin(); i != clusterings[0].end(); i++)
    os << i->first << "\n";
  for(uint i = 0; i < num_clusterings; i++)
    os << "clustering" << i << "\n" << clusterings[i] << ".\n";
}

// read a vector of clusterings from a file
template <typename T>
vector<clustering<T> > read_clusterings_from_file(const string& filename){
  vector<clustering<T> > result;
  ifstream fin;
  fin.open(filename.c_str());
  if(fin.good()) result = read_clusterings<T>(fin);
  return result;
}

// write a vector of clusterings to a file, return success
template <typename T>
bool write_clusterings_to_file(const string& filename, const vector<clustering<T> > clusterings){
  ofstream fout;
  fout.open(filename.c_str());
  if(fout.good()) {
    write_clusterings<T>(fout, clusterings);
    return true;
  } else return false;
}

// write a clustering to a file, return success
template <typename T>
bool write_clustering_to_file(const string& filename, const clustering<T> C){
  ofstream fout;
  fout.open(filename.c_str());
  if(fout.good()) {
    fout << C;
    return true;
  } else return false;
}

// read a vector of clusterings from a file
template <typename T>
clustering<T> generate_random_clustering(const set<T>& elements, const bool allow_unclustered = false){
  clustering<T> result;
  uint max_cluster = 0;
  uint cluster;
  for(typename set<T>::const_iterator i = elements.begin(); i != elements.end(); i++){
    if(allow_unclustered)
      cluster = (uint)floor(((double)(max_cluster + 2) * rand())/((double)RAND_MAX + 1));
    else
      cluster = 1 + (uint)floor(((double)(max_cluster + 1) * rand())/((double)RAND_MAX + 1));

    if(cluster > max_cluster) max_cluster = cluster;
    result.insert(pair<T,uint>(*i,cluster));
  }
  return result;
}


template <typename T>
ostream& operator<<(ostream& os, const clustering<T>& C){
  typename set<T>::const_iterator k;
  typename map<uint,set<T> >::const_iterator j;
  map<uint, set<T> > clusters;

  clusters = map<uint, set<T> >();
  for(typename clustering<T>::const_iterator i = C.begin(); i != C.end(); i++){
    if(clusters.find(i->second) == clusters.end())
      clusters.insert(pair<uint,set<T> >(i->second,set<T>()));
    clusters[i->second].insert(i->first);
  }
  j = clusters.begin();
  while(j != clusters.end()){
    if(j->first && (!j->second.empty())){
      k = j->second.begin();
      os << *(k++);
      while(k != j->second.end()) os << "," << *(k++);
      j++;
      os << ";";
    } else j++;
  }
  return os;
}

template <typename T>
ostream& operator<<(ostream& os, const vector<clustering<T> >& C){
  typename vector<clustering<T> >::const_iterator i;
  if(C.size()){
    i = C.begin();
    os << *(i++);
    while(i != C.end())
      os << "\n" << *(i++);
  }
  return os;
}

template <typename T>
ostream& operator<<(ostream& os, const set<T>& S){
  typename set<T>::const_iterator i = S.begin();
  os << "{";
  if(!S.empty()){
    os << *(i++);
    while(i != S.end()) os << "," << *(i++);
  }
  os << "}";
  return os;
}
#endif
