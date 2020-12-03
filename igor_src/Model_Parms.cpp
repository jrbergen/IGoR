/*
 * Model_Parms.cpp
 *
 *  Created on: 3 nov. 2014
 *      Author: Quentin Marcou
 *
 *  This source code is distributed as part of the IGoR software.
 *  IGoR (Inference and Generation of Repertoires) is a versatile software to analyze and model immune receptors
 *  generation, selection, mutation and all other processes.
 *   Copyright (C) 2017  Quentin Marcou
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *      This class defines a Model_Parms object. In our framework this will be used
 *      to easily define the inferred model, by using a directed graph structure.
 *      This helps defining conditional marginals in the model.
 */

#include "Model_Parms.h"
using namespace std;

/*
 * Default constructor, creates an empty Model_Parms
 */
Model_Parms::Model_Parms() {
/*	this->events = *(new list <shared_ptr<Rec_Event>>()); //FIXME nonsense new
	this->edges = *(new unordered_map <Rec_Event_name , Adjacency_list >());*/

}

/*
 * Construct a Model_Parms from a list of event and initialize connectivities/Adjacency_lists
 */
Model_Parms::Model_Parms(list <shared_ptr<Rec_Event>> event_list){
	this->events = event_list;
	//this->edges = *(new unordered_map<Rec_Event_name,Adjacency_list>()); //FIXME nonsense new
	size_t event_identifier = 0;
	for(list<shared_ptr<Rec_Event>>::const_iterator iter = this->events.begin() ; iter != this->events.end() ; ++iter){
		this->edges.emplace( (*iter)->get_name() , Adjacency_list());
		(*iter)->set_event_identifier(event_identifier);
		++event_identifier;
	}
}

/*
 * Provide a deep copy constructor of the Model_Parms object
 * This method will make a copy of all the contained events and error rate as well
 */
Model_Parms::Model_Parms(const Model_Parms& other){
	for(list<shared_ptr<Rec_Event>>::const_iterator iter = other.events.begin() ; iter!= other.events.end() ; ++iter){
		this->events.push_back((*iter)->copy());
	}

	for(unordered_map <Rec_Event_name , Adjacency_list >::const_iterator iter = other.edges.begin() ; iter != other.edges.end() ; ++iter){
		Adjacency_list adjacency_list;
		//TODO very dirty need to be changed
		for(list<shared_ptr<Rec_Event>>::const_iterator jiter = (*iter).second.children.begin() ; jiter != (*iter).second.children.end() ; ++jiter){
			for(list<shared_ptr<Rec_Event>>::const_iterator kiter = this->events.begin() ; kiter != this->events.end() ; kiter++){
				if((**kiter)==(**jiter)){
					adjacency_list.children.push_back((*kiter));
					break;
				}
			}
		}
		for(list<shared_ptr<Rec_Event>>::const_iterator jiter = (*iter).second.parents.begin() ; jiter != (*iter).second.parents.end() ; ++jiter){
			for(list<shared_ptr<Rec_Event>>::const_iterator kiter = this->events.begin() ; kiter != this->events.end() ; ++kiter){
				if((**kiter)==(**jiter)){
					adjacency_list.parents.push_back((*kiter));
					break;
				}
			}
		}

		this->edges.emplace((*iter).first,adjacency_list);
	}
	this->error_rate = other.error_rate->copy();
}




/*
 * Provide a deep copy of the model parameters
 */
/*Model_Parms::Model_Parms(const Model_Parms& other){
	this->error_rate = other.error_rate->copy();
	for(unordered_map<Rec_Event_name , Adjacency_list>::const_iterator iter=other.edges.begin() ; iter != other.edges.end();iter++){
		this->edges.emplace((*iter).first,*iter.)
	 }

	for(list<Rec_Event*>::const_iterator iter = other.events.begin() ; iter != other.events.end() ; iter++){
		this->events.emplace_back((*iter)->copy());
	}
}*/


/*
 * Avoid memory leaks by deleting all events
 */
Model_Parms::~Model_Parms() {
/*	for(list<Rec_Event*>::iterator iter = events.begin() ; iter != events.end() ; iter++){
		delete (*iter);
	}
	delete error_rate;*/
}



/*
 * Add an event to the event list, and initialize its connectivity
 */
bool Model_Parms::add_event(shared_ptr<Rec_Event> event_point){
	//this->events.push_back(event_point);
	event_point->set_event_identifier(this->events.size());
	this->events.emplace_back(event_point);
	this->edges.emplace(event_point->get_name(), Adjacency_list());
	return 1;
}
bool Model_Parms::add_event(Rec_Event* event_point){
	return this->add_event(shared_ptr<Rec_Event>(event_point,null_delete<Rec_Event>()));
}



/*
 * Get the list of parents of the given Rec_event
 */
list<shared_ptr<Rec_Event>> Model_Parms::get_parents(Rec_Event* event) const{
	return this->get_parents(shared_ptr<Rec_Event>(event,null_delete<Rec_Event>()));
}
list<shared_ptr<Rec_Event>> Model_Parms::get_parents(shared_ptr<Rec_Event> event) const{
	return this->get_parents(event->get_name());
}
list<shared_ptr<Rec_Event>> Model_Parms::get_parents(Rec_Event_name event_name) const{
	if(edges.count(event_name)<=0){
		throw runtime_error("Model_Parms::get_parents(): event \"" + event_name + "\" does not exist in \"this\".");
	}
	return edges.at(event_name).parents;
}

/*
 * Get the list of children of the given Rec_event
 */
list<shared_ptr<Rec_Event>> Model_Parms::get_children(Rec_Event* event) const{
	return this->get_children(shared_ptr<Rec_Event>(event,null_delete<Rec_Event>()));
}
list<shared_ptr<Rec_Event>> Model_Parms::get_children(shared_ptr<Rec_Event> event) const{
	return this->get_children(event->get_name());
}
list<shared_ptr<Rec_Event>> Model_Parms::get_children(Rec_Event_name event_name) const{
	if(edges.count(event_name)<=0){
		throw runtime_error("Model_Parms::get_children(): event \"" + event_name + "\" does not exist in \"this\".");
	}
	return edges.at(event_name).children;
}

/**
 * \overload list<shared_ptr<Rec_Event>> Model_Parms::get_ancestors(Rec_Event_name event_name) const
 */
list<shared_ptr<Rec_Event>> Model_Parms::get_ancestors(Rec_Event* event) const{
	return this->get_ancestors(shared_ptr<Rec_Event>(event,null_delete<Rec_Event>()));
}
/**
 * \overload list<shared_ptr<Rec_Event>> Model_Parms::get_ancestors(Rec_Event_name event_name) const
 */
list<shared_ptr<Rec_Event>> Model_Parms::get_ancestors(shared_ptr<Rec_Event> event) const{
	return this->get_ancestors(event->get_name());
}
/**
 * \brief Get all ancestors of the supplied event
 * \author Q.Marcou
 * \version 1.0
 * \param[in] event_name The event whom we seek the ancestors
 * \return The list of the event ancestors
 *
 *  Get all ancestors of the supplied event. Ancestors are all events higher than the considered event
 *  in the genealogy and with link of any degree to it
 */
list<shared_ptr<Rec_Event>> Model_Parms::get_ancestors(Rec_Event_name event_name) const{
	set<shared_ptr<Rec_Event>> ancestor_set;
	list<shared_ptr<Rec_Event>> exploratory_list;
	list<shared_ptr<Rec_Event>> final_list;

	//Get the considered event parent
	if(edges.count(event_name)<=0){
		throw runtime_error("Model_Parms::get_parents(): event \"" + event_name + "\" does not exist in \"this\".");
	}
	exploratory_list = edges.at(event_name).parents;

	//Now explore all ancestors
	while(not exploratory_list.empty()){
		//If the ancestor is not already in the list
		if(ancestor_set.count(exploratory_list.front())<=0){
			final_list.emplace_back(exploratory_list.front());
			ancestor_set.emplace(exploratory_list.front());
			const list<shared_ptr<Rec_Event>>& parents = edges.at(exploratory_list.front()->get_name()).parents;;
			exploratory_list.insert(exploratory_list.end(),parents.begin(),parents.end());
		}
		exploratory_list.pop_front();
	}

	return final_list;
}

/*
 * Add a directed edge between two events
 */
bool Model_Parms::add_edge(Rec_Event* parent_point, Rec_Event* child_point){
	shared_ptr<Rec_Event> parent_smart_p (parent_point,null_delete<Rec_Event>());
	shared_ptr<Rec_Event> child_smart_p (child_point,null_delete<Rec_Event>());
	return this->add_edge(parent_smart_p,child_smart_p);
}
/**
 * \bug this is not checking whether the exact same edge already exists
 */
bool Model_Parms::add_edge(shared_ptr<Rec_Event> parent_point, shared_ptr<Rec_Event> child_point){
	// check whether these events exist
	if( edges.count(parent_point->get_name())<=0 ){
		throw runtime_error("Model_Parms::add_edge(): event \"" + parent_point->get_name() + "\" does not exist in \"this\".");
	}
	if( edges.count(child_point->get_name())<=0 ){
		throw runtime_error("Model_Parms::add_edge(): event \"" + child_point->get_name() + "\" does not exist in \"this\".");
	}
	//Check whether it is creating a cycle
		//First check the silly self-loop cycle
		if(parent_point->get_name() == child_point->get_name()){
			throw runtime_error("Trying to create an edge between " + parent_point->get_name() + "and itself");
		}

		//Else check if the new children is an ancestor of the new parent (this would close a cycle)
		list<shared_ptr<Rec_Event>> parent_ancestors = this->get_ancestors(parent_point);
		for(const shared_ptr<Rec_Event> event_ptr : parent_ancestors){
			if(child_point->get_name() == event_ptr->get_name()){
				throw runtime_error(child_point->get_name() + " is an ancestor of " + parent_point->get_name() +
						" adding an edge would create a cycle in the graph, in Model_Parms::add_edge");
			}
		}


	this->edges.at( parent_point->get_name() ).children.push_back(child_point);
	this->edges.at( child_point->get_name() ).parents.push_back(parent_point);
	return 1;
}

bool Model_Parms::add_edge(Rec_Event_name parent_name, Rec_Event_name child_name){
	shared_ptr<Rec_Event> parent_smart_p = this->get_event_pointer(parent_name);
	shared_ptr<Rec_Event> child_smart_p = this->get_event_pointer(child_name);
	return this->add_edge(parent_smart_p , child_smart_p);
}

/*
 * Remove the corresponding edge if it exists
 */
bool Model_Parms::remove_edge(Rec_Event* parent_point, Rec_Event* child_point){
	shared_ptr<Rec_Event> parent_smart_p (parent_point,null_delete<Rec_Event>());
	shared_ptr<Rec_Event> child_smart_p (child_point,null_delete<Rec_Event>());
	return this->remove_edge(parent_smart_p,child_smart_p);
}
bool Model_Parms::remove_edge(shared_ptr<Rec_Event> parent_point, shared_ptr<Rec_Event> child_point){
	if(this->has_edge(parent_point,child_point)){
		list<shared_ptr<Rec_Event>>::iterator iter;

		//Remove the child from the parent's children list
		list<shared_ptr<Rec_Event>>& children_list = this->edges.at(parent_point->get_name()).children;
		for( iter = children_list.begin() ;
				iter != children_list.end() ;
				++iter){
			//Compare the pointers and stop when finding the correct one
			if((*iter) == child_point){
				break;
			}
		}
		//Erase the pointer using the iterator
		children_list.erase(iter);

		//Remove the parent from the child's parent list
		list<shared_ptr<Rec_Event>>& parents_list = this->edges.at(child_point->get_name()).parents;
		for( iter = parents_list.begin() ;
				iter != parents_list.end() ;
				++iter){
			//Compare the pointers and stop when finding the correct one
			if((*iter) == parent_point){
				break;
			}
		}
		//Erase the pointer using the iterator
		parents_list.erase(iter);
		return 1;
	}
	else{
		throw runtime_error("Model_Parms::remove_edge(): edge between \"" + parent_point->get_name() + "\" and \"" + child_point->get_name() + "\" does not exist.");
	}
}

bool Model_Parms::remove_edge(Rec_Event_name parent_name, Rec_Event_name child_name){
	shared_ptr<Rec_Event> parent_smart_p = this->get_event_pointer(parent_name);
	shared_ptr<Rec_Event> child_smart_p = this->get_event_pointer(child_name);
	return this->remove_edge(parent_smart_p , child_smart_p);
}

/*
 * Inverse the direction of an edge between two events
 * Detects automatically the edge direction invert it
 */
void Model_Parms::invert_edge(Rec_Event* ev1_point, Rec_Event* ev2_point){
	shared_ptr<Rec_Event> ev1_smart_p (ev1_point,null_delete<Rec_Event>());
	shared_ptr<Rec_Event> ev2_smart_p (ev2_point,null_delete<Rec_Event>());
	this->invert_edge(ev1_smart_p,ev2_smart_p);
}

void Model_Parms::invert_edge(shared_ptr<Rec_Event> ev1_point, shared_ptr<Rec_Event> ev2_point){

	//First check if ev2 is a child of ev1
	if(this->has_edge(ev1_point,ev2_point)){
		//Invert the edge
		this->remove_edge(ev1_point,ev2_point);
		try{
			this->add_edge(ev2_point,ev1_point);
		}
		catch(exception& e){
			//add_edge checks for cycle creation since 13/04/2017
			cerr<<"Exception caught trying to invert an edge between " + ev1_point->get_name() +" and " + ev2_point->get_name() +" in Model_Parms::invert_edge , this is most likely creating a cycle, throwing exception now..."<<endl;
			throw e;
		}

	}
	//Otherwise check if ev2 is a parent of ev1
	else if(this->has_edge(ev2_point,ev1_point)){
		//Invert the edge
		this->remove_edge(ev2_point,ev1_point);
		try{
			this->add_edge(ev1_point,ev2_point);
		}
		catch(exception& e){
			//add_edge checks for cycle creation since 13/04/2017
			cerr<<"Exception caught trying to invert an edge between " + ev1_point->get_name() +" and " + ev2_point->get_name() +" in Model_Parms::invert_edge , this is most likely creating a cycle, throwing exception now..."<<endl;
			throw e;
		}
	}
	else{
		//else: the edge do not exist and throw an exception
		throw runtime_error("In Model_Parms::invert_edge(): no edge exist between \""
				+ ev1_point->get_name() +"\" and \"" + ev1_point->get_name() + "\" events in any direction");
	}
}

void Model_Parms::invert_edge(Rec_Event_name ev1_name, Rec_Event_name ev2_name){
	shared_ptr<Rec_Event> ev1_smart_p = this->get_event_pointer(ev1_name);
	shared_ptr<Rec_Event> ev2_smart_p = this->get_event_pointer(ev2_name);
	return this->invert_edge(ev1_smart_p , ev2_smart_p);
}

/*
 * Check if the model parms contain a given directed edge
 *
 * Note: assuming the edge has been correctly constructed it will test the existence in only one of the adjacency list
 */
bool Model_Parms::has_edge(Rec_Event* parent_point, Rec_Event* child_point) const{
	shared_ptr<Rec_Event> parent_smart_p (parent_point,null_delete<Rec_Event>());
	shared_ptr<Rec_Event> child_smart_p (child_point,null_delete<Rec_Event>());
	return this->has_edge(parent_smart_p,child_smart_p);
}

bool Model_Parms::has_edge(shared_ptr<Rec_Event> parent_point, shared_ptr<Rec_Event> child_point) const{
	return this->has_edge(parent_point->get_name(),child_point->get_name());
}

bool Model_Parms::has_edge(Rec_Event_name parent_name, Rec_Event_name child_name) const{
	bool other_event_found = false;
	const Adjacency_list& adjacency_list = this->edges.at(parent_name);
	for(shared_ptr<Rec_Event> ev_ptr : adjacency_list.children){
		if(ev_ptr->get_name() == child_name){
			other_event_found = true;
		}
	}
	return other_event_found;
}


/*
 * This method returns all the roots of the tree (events with no parents).
 */
list<shared_ptr<Rec_Event>> Model_Parms::get_roots() const{
	list<shared_ptr<Rec_Event>> root_list = list<shared_ptr<Rec_Event>> (); //FIXME nonsense new
	for ( list <shared_ptr<Rec_Event>>::const_iterator iter = this->events.begin(); iter != this->events.end(); ++iter){
		if (this->edges.at( (*iter)->get_name() ).parents.empty()){
			root_list.push_back(*iter);
		}
	}
	// root events are sorted for easier manipulation
	root_list.sort(Event_comparator());
	return root_list;
}



/*
 * This method output the queue (order in which events are processed) according to the structure of the model
 * This queue is used both for the order of iteration and the design of the marginal array.
 */
queue<shared_ptr<Rec_Event>> Model_Parms::get_model_queue() const{

	list<shared_ptr<Rec_Event>> model_roots = this->get_roots();
	list<shared_ptr<Rec_Event>> events_copy_list = this->events;


	queue<shared_ptr<Rec_Event>> model_queue = queue<shared_ptr<Rec_Event>> ();


	//if all events are root they are independent and thus the queue is sorted only by priority
	if(events_copy_list.size() == model_roots.size()){
		//root list is already sorted by priority
		for(list<shared_ptr<Rec_Event>>::const_iterator iter=model_roots.begin(); iter!=model_roots.end(); ++iter){
			model_queue.push(*iter);
		}
		return model_queue;
	}

	//The root with highest priority is the first event
	events_copy_list.remove(*(model_roots.begin()));
	model_queue.push(*(model_roots.begin()));

	//Keep track of the events already added to the queue
	unordered_map<Rec_Event_name,shared_ptr<Rec_Event>>* processed_events_point = new unordered_map<Rec_Event_name,shared_ptr<Rec_Event>>;
	(*processed_events_point).insert(make_pair( (*model_roots.begin())->get_name() , *(model_roots.begin()) ));
	model_roots.pop_front();

	events_copy_list.sort(Event_comparator());
	list<shared_ptr<Rec_Event>>::iterator iter = events_copy_list.begin();

	while(!events_copy_list.empty()){

		//Before an event is processed his parents must have been processed, this overrides the priority!

		list<shared_ptr<Rec_Event>> event_parents = this->edges.at( (*iter)->get_name() ).parents;
		bool parents_processed = 1;
		for(list<shared_ptr<Rec_Event>>::const_iterator jiter = event_parents.begin() ; jiter != event_parents.end() ; ++jiter){
			if((*processed_events_point).count( (*jiter)->get_name() ) ==0 ) {parents_processed=0;}
		}
		//If all parents have not been processed, tries the next highest priority event
		if(!parents_processed){
			++iter;
			continue;
		}

		model_queue.push(*iter);
		(*processed_events_point).insert(make_pair( (*iter)->get_name() , (*iter) ));
		events_copy_list.erase(iter);
		events_copy_list.sort(Event_comparator()); //TODO again check this sort function
		iter = events_copy_list.begin();

	}
	delete processed_events_point;
	return model_queue;



}

shared_ptr<Rec_Event> Model_Parms::get_event_pointer(const string& event_str , bool by_nickname) const{
	//TODO find something better, might be a bottleneck
	for (list<shared_ptr<Rec_Event>>::const_iterator iter = this->events.begin() ; iter != this->events.end() ; ++iter){
		if(by_nickname){
			if ((*iter)->get_nickname() == event_str){return (*iter);}
		}
		else{
			if ((*iter)->get_name() == event_str){return (*iter);}
		}
	}
	if(by_nickname){
		throw runtime_error("Event pointer not found in Model_Parms::get_event_pointer for nickname:" + event_str);
	}
	else{
		throw runtime_error("Event pointer not found in Model_Parms::get_event_pointer for name:" + event_str);
	}
}

shared_ptr<Rec_Event> Model_Parms::get_event_pointer(const Rec_Event_name& event_name) const{
	return this->get_event_pointer(event_name,false);
}

void Model_Parms::update_edge_event_name(Rec_Event_name former_name , Rec_Event_name new_name){
	if(former_name != new_name){
		Adjacency_list adjacency_list =  this->edges.at(former_name);
		this->edges.emplace(new_name , adjacency_list);
		this->edges.erase(former_name);
	}
}

const unordered_map<tuple<Event_type,Gene_class,Seq_side>, shared_ptr<Rec_Event>> Model_Parms::get_events_map() const{
	unordered_map<tuple<Event_type,Gene_class,Seq_side>, shared_ptr<Rec_Event>> events_map;
	for(list<shared_ptr<Rec_Event>>::const_iterator iter = this->events.begin() ; iter != this->events.end() ; ++iter ){
		events_map.emplace(tuple<Event_type,Gene_class,Seq_side>( (*iter)->get_type() , (*iter)->get_class() , (*iter)->get_side() ) , (*iter));
	}
	return events_map;
}

unordered_map<tuple<Event_type,Gene_class,Seq_side>, shared_ptr<Rec_Event>> Model_Parms::get_events_map() {
	unordered_map<tuple<Event_type,Gene_class,Seq_side>, shared_ptr<Rec_Event>> events_map;
	for(list<shared_ptr<Rec_Event>>::const_iterator iter = this->events.begin() ; iter != this->events.end() ; ++iter ){
		events_map.emplace(tuple<Event_type,Gene_class,Seq_side>( (*iter)->get_type() , (*iter)->get_class() , (*iter)->get_side() ) , (*iter));
	}
	return events_map;
}

void Model_Parms::write_model_parms(string filename){
	ofstream outfile(filename);
	outfile<<"@Event_list"<<endl;
	for(list<shared_ptr<Rec_Event>>::const_iterator iter=events.begin() ; iter != events.end() ; ++iter){
		(*iter)->write2txt(outfile);
	}
	outfile<<"@Edges"<<endl;
	for(list<shared_ptr<Rec_Event>>::const_iterator iter=events.begin() ; iter != events.end() ; ++iter){
		list<shared_ptr<Rec_Event>> children = edges[(*iter)->get_name()].children;
		for(list<shared_ptr<Rec_Event>>::const_iterator jiter=children.begin() ; jiter!=children.end() ; ++jiter){
			outfile<<"%"<<(*iter)->get_name()<<";"<<(*jiter)->get_name()<<endl;
		}
	}
	outfile<<"@ErrorRate"<<endl;
	error_rate->write2txt(outfile);
}

void Model_Parms::read_model_parms(string filename){
	ifstream infile(filename);
	if(!infile){
		//Throw exception
		throw runtime_error("File not found : \""+filename + "\"");
	}
	string line_str;
	getline(infile,line_str);
	if(line_str == string("@Event_list")){
		getline(infile,line_str);
		while(line_str[0] == '#'){
			size_t semicolon_index = line_str.find(";",0);
			string event = line_str.substr(1,semicolon_index-1);
			size_t next_semicolon_index = line_str.find(";",semicolon_index+1);
			string event_class_str = line_str.substr(semicolon_index+1,(next_semicolon_index - semicolon_index - 1));
			Gene_class event_class;
			try{
				event_class = str2GeneClass(event_class_str);
			}
			catch(exception& e){
				throw runtime_error("Unknown Gene_class\""+event_class_str +"\" in model file: \"" + filename + "\"");
			}

			semicolon_index = next_semicolon_index;
			next_semicolon_index = line_str.find(";",semicolon_index+1);
			string event_side_str = line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index - 1));
			Seq_side event_side;
			try{
				event_side = str2SeqSide(event_side_str);
			}
			catch(exception& e){
				throw runtime_error("Unknown Seq_side\""+event_side_str +"\" in file: \"" + filename + "\"");
			}


			semicolon_index = next_semicolon_index;
			next_semicolon_index = line_str.find(";",semicolon_index+1);
			int priority;
			string nickname;
			if(next_semicolon_index==string::npos){
				//For retro-compatibility purposes
				priority = stoi(line_str.substr(semicolon_index+1,string::npos));
			}
			else{
				//Read the nickname along with the priority
				priority = stoi(line_str.substr(semicolon_index+1,next_semicolon_index-1));
				nickname = line_str.substr(next_semicolon_index+1,string::npos);
			}


			cerr<<event<<" read"<<endl;
			if(event == string("Insertion")){
				unordered_map<string,Event_realization> event_realizations = unordered_map<string,Event_realization> ();
				getline(infile,line_str);
				while(line_str[0]=='%'){
					semicolon_index = line_str.find(";",0);
					int value_int = stoi(line_str.substr(1,semicolon_index));
					int index = stoi(line_str.substr(semicolon_index+1,string::npos));
					event_realizations.emplace(pair<string,Event_realization>(to_string(value_int) , Event_realization(to_string(value_int) , value_int , "",Int_Str() , index)));
					getline(infile,line_str);
				}
				//TODO check this for enum writing
				//Insertion new_event = Insertion(event_class , event_side , event_realizations);
				shared_ptr<Insertion> new_event_p =  shared_ptr<Insertion> (new Insertion(event_class  , event_realizations));
				new_event_p->set_priority(priority);
				new_event_p->set_nickname(nickname);
				this->add_event(new_event_p);
			}
			else if(event == string("Deletion")){
				unordered_map<string,Event_realization> event_realizations = unordered_map<string,Event_realization> ();
				getline(infile,line_str);
				while(line_str[0]=='%'){
					semicolon_index = line_str.find(";",0);
					int value_int = stoi(line_str.substr(1,semicolon_index));
					int index = stoi(line_str.substr(semicolon_index+1,string::npos));
					event_realizations.emplace(pair<string,Event_realization>(to_string(value_int) , Event_realization(to_string(value_int) , value_int , "",Int_Str() , index))) ;
					getline(infile,line_str);
				}
				//TODO check this for enum writing
				//Deletion new_event = Deletion(event_class , event_side , event_realizations);
				shared_ptr<Deletion> new_event_p =shared_ptr<Deletion> (new Deletion(event_class , event_side , event_realizations));
				new_event_p->set_priority(priority);
				new_event_p->set_nickname(nickname);
				this->add_event(new_event_p);
			}
			else if(event == string("GeneChoice")){
				unordered_map<string,Event_realization> event_realizations = unordered_map<string,Event_realization> (); //FIXME nonsense new
				getline(infile,line_str);
				while(line_str[0]=='%'){
					semicolon_index = line_str.find(";",0);
					string name = line_str.substr(1,semicolon_index-1);
					next_semicolon_index = line_str.find(";",semicolon_index+1);
					string value_str = line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1) );
					string test = line_str.substr(next_semicolon_index+1,string::npos);
					int index = stoi(line_str.substr(next_semicolon_index+1,string::npos));
					event_realizations.emplace(pair<string,Event_realization>(name,Event_realization(name , INT16_MAX , value_str , nt2int(value_str) , index)));
					getline(infile,line_str);
				}
				//TODO check this for enum writing
				//Gene_choice new_event = Gene_choice(event_class , event_side , event_realizations);
				shared_ptr<Gene_choice> new_event_p = shared_ptr<Gene_choice> (new Gene_choice(event_class  , event_realizations));//TODO construct event before and use add realization instead?
				new_event_p->set_priority(priority);
				new_event_p->set_nickname(nickname);
				this->add_event(new_event_p);
			}
			else if(event== string("DinucMarkov")){
				shared_ptr<Dinucl_markov> new_event_p = shared_ptr<Dinucl_markov> (new Dinucl_markov(event_class));
				new_event_p->set_priority(priority);
				new_event_p->set_nickname(nickname);
				this->add_event(new_event_p);
				getline(infile,line_str);
				while(line_str[0]=='%'){
					getline(infile,line_str);
				}
			}
			else{
				throw runtime_error(event + " event is not implemented (thrown by Model_Parms::read_model_parms");
			}



		}
	}
	else{
		throw runtime_error("Unknown format for model_parms file");
	}
	if(line_str == string("@Edges")){
		getline(infile,line_str);
		while(line_str[0] == '%'){
			size_t semicolon_index = line_str.find(";",0);
			string parent_name = line_str.substr(1,semicolon_index-1);
			string child_name = line_str.substr(semicolon_index+1 , string::npos);
			edges[parent_name].children.emplace_back(get_event_pointer(child_name));
			edges[child_name].parents.emplace_back(get_event_pointer(parent_name));
			getline(infile , line_str);
		}
	}
	else{
		throw runtime_error("Unknown format for model file: " + filename);
	}
	if(line_str == string("@ErrorRate")){
		getline(infile,line_str);
		size_t semicolon_index = line_str.find(";");
		string errrate = line_str.substr(0,semicolon_index);
		if(errrate == string("#SingleErrorRate")){
			getline(infile,line_str);
			shared_ptr<Single_error_rate> err_rate_p = shared_ptr<Single_error_rate> (new Single_error_rate(stod(line_str)));
			this->error_rate = err_rate_p;
		}
		else if(errrate == string("#Hypermutationglobalerrorrate")){

			//Get the general attributes for the hypermutation model (size,learn_on,apply_on)
			size_t next_semicolon_index = line_str.find(";",semicolon_index+1);
			size_t mutation_Nmer_size = stoi(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)));
			semicolon_index = next_semicolon_index;
			next_semicolon_index = line_str.find(";",semicolon_index+1);
			Gene_class learn_on;
			try{
				learn_on = str2GeneClass(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)));
			}
			catch(exception& e){
				throw runtime_error("Unknown Gene_class\""+ line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)) +"\" for Hypermutationglobalerrorrate in model file: " + filename);
			}
			semicolon_index = next_semicolon_index;
			next_semicolon_index = line_str.find(";",semicolon_index+1);
			Gene_class apply_on;
			try{
				apply_on = str2GeneClass(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)));
			}
			catch(exception& e){
				throw runtime_error("Unknown Gene_class\""+ line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)) +"\" for Hypermutationglobalerrorrate in model file: " + filename);
			}

			//Get the global rate
			getline(infile,line_str);
			double R = stod(line_str);

			//Get the contributions
			getline(infile,line_str);
			semicolon_index = 0;
			next_semicolon_index = line_str.find(";");
			vector<double> ei_contributions ;
			while( semicolon_index!=string::npos ){
				if(semicolon_index==0){
					ei_contributions.push_back(stod(line_str.substr(semicolon_index , (next_semicolon_index - semicolon_index ))));
				}
				else{
					ei_contributions.push_back(stod(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1))));
				}
				semicolon_index = next_semicolon_index;
				next_semicolon_index = line_str.find(";",semicolon_index+1);
			}
			shared_ptr<Hypermutation_global_errorrate> err_rate_p = shared_ptr<Hypermutation_global_errorrate> (new Hypermutation_global_errorrate(mutation_Nmer_size, learn_on , apply_on , R , ei_contributions));
			this->set_error_ratep(err_rate_p);
		}
		else if(errrate == string("#HypermutationfullNmererrorrate")){
			//Get the general attributes for the hypermutation model (size,learn_on,apply_on)
			size_t next_semicolon_index = line_str.find(";",semicolon_index+1);
			size_t mutation_Nmer_size = stoi(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)));
			semicolon_index = next_semicolon_index;
			next_semicolon_index = line_str.find(";",semicolon_index+1);
			Gene_class learn_on;
			try{
				learn_on = str2GeneClass(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)));
			}
			catch(exception& e){
				throw runtime_error("Unknown Gene_class\""+ line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)) +"\" for Hypermutationglobalerrorrate in model file: " + filename);
			}
			semicolon_index = next_semicolon_index;
			next_semicolon_index = line_str.find(";",semicolon_index+1);
			Gene_class apply_on;
			try{
				apply_on = str2GeneClass(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)));
			}
			catch(exception& e){
				throw runtime_error("Unknown Gene_class\""+ line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1)) +"\" for Hypermutationglobalerrorrate in model file: " + filename);
			}

			//Get the mutation probas
			getline(infile,line_str);
			semicolon_index = 0;
			next_semicolon_index = line_str.find(";");
			vector<double> mutation_probas ;
			while( semicolon_index!=string::npos ){
				if(semicolon_index==0){
					mutation_probas.push_back(stod(line_str.substr(semicolon_index , (next_semicolon_index - semicolon_index ))));
				}
				else{
					mutation_probas.push_back(stod(line_str.substr(semicolon_index+1 , (next_semicolon_index - semicolon_index -1))));
				}
				semicolon_index = next_semicolon_index;
				next_semicolon_index = line_str.find(";",semicolon_index+1);
			}
			shared_ptr<Hypermutation_full_Nmer_errorrate> err_rate_p = shared_ptr<Hypermutation_full_Nmer_errorrate> (new Hypermutation_full_Nmer_errorrate(mutation_Nmer_size, learn_on , apply_on , mutation_probas));
			this->set_error_ratep(err_rate_p);
		}
		else{
			throw runtime_error("Unknown Error_rate type\""+ errrate + "\" in model file: " + filename);
		}

	}
	else{
		throw runtime_error("Unknown format for model file: " + filename);
	}


}

void Model_Parms::set_fixed_all_events(bool fix_bool_status){
	for(list<shared_ptr<Rec_Event>>::iterator iter = events.begin() ; iter != events.end() ; ++iter){
		(*iter)->fix(fix_bool_status);
	}
}
