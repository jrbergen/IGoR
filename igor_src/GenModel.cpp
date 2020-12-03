/*
 * GenModel.cpp
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
 *      This class designs a generative model and supply all the methods to run a maximum likelihood estimate of the generative model
 */

#include "GenModel.h"

using namespace std;

GenModel::GenModel(const Model_Parms& parms, const Model_marginals& marginals, const map<size_t,shared_ptr<Counter>>& count_list): model_parms(parms) , model_marginals(marginals) , counters_list(count_list){}

GenModel::GenModel(const Model_Parms& parms, const Model_marginals& marginals):GenModel(parms , marginals , map<size_t,shared_ptr<Counter>>()){}

GenModel::GenModel(const Model_Parms& parms): GenModel(parms ,  Model_marginals(parms) , map<size_t,shared_ptr<Counter>>())  {
}

GenModel::~GenModel() {
	// TODO Auto-generated destructor stub
}


bool GenModel::infer_model(const vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>& sequences ,const  int iterations ,const std::string path, bool fast_iter , double likelihood_threshold/*=1e-25*/ , bool viterbi_like/*false*/){
	return this->infer_model(sequences , iterations , path , fast_iter , likelihood_threshold , viterbi_like , 0.001);
}

bool GenModel::infer_model(const vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>& sequences ,const  int iterations ,const std::string path, bool fast_iter/*=true*/ , double likelihood_threshold/*=1e-25*/ , double proba_threshold_factor/*=0.001*/ ){
	return this->infer_model(sequences , iterations , path , fast_iter , likelihood_threshold , false , proba_threshold_factor , INFINITY);
}

bool GenModel::infer_model(const vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>& sequences ,const  int iterations ,const string path , bool fast_iter/*=true*/ ,double likelihood_threshold/*=1e-25 by default*/ , bool viterbi_like/*=false*/ , double proba_threshold_factor/*=0.001 by default*/ , double mean_number_seq_err_thresh /*= INFINITY by default*/){

	//If viterbi like only the best scenario is of interest
	if(viterbi_like){
		cerr<<"******************************************************************"<<endl;
		cerr<<"*\t\t RUNNING \"VITERBI\" LIKE ALGORITHM \t\t *"<<endl<<"* \t(only the best scenario will be taken into account)\t *"<<endl;
		cerr<<"******************************************************************"<<endl;
		proba_threshold_factor = 1.0;
	}

	if(likelihood_threshold>1.0){
		throw invalid_argument("Likelihood threshold must be lesser or equal than one");
	}

	if(proba_threshold_factor>1.0){
		throw invalid_argument("Probability threshold ratio must be lesser or equal than one");
	}

	ofstream likelihood_file(path + "likelihoods.out");
	likelihood_file<<"iteration;mean_log_Likelihood;n_seq"<<endl;


	queue<shared_ptr<Rec_Event>> model_queue = model_parms.get_model_queue();
	unordered_map<Rec_Event_name,int> index_map = model_marginals.get_index_map(model_parms,model_queue);
	unordered_map<Rec_Event_name,list<pair<shared_ptr<const Rec_Event>,int>>> inv_offset_map = model_marginals.get_inverse_offset_map(model_parms,model_queue);
	int iteration_accomplished = 0;
	ofstream log_file(path + string("inference_logs.txt"));
	log_file<<"iteration_n;seq_processed;seq_index;nt_sequence;n_V_aligns;n_J_aligns;seq_likelihood;seq_mean_n_errors;seq_n_scenarios;seq_best_scenario;time"<<endl;
	ofstream general_logs(path + string("inference_info.out"));
	//Dump all inference parameters to file
	chrono::system_clock::time_point begin_time = chrono::system_clock::now();
	std::time_t tt;
	tt = chrono::system_clock::to_time_t ( begin_time );

	general_logs<<"Date: "<< ctime(&tt)<<endl;
	general_logs<<"Max #iterations to be performed: "<<iterations<<endl;
	general_logs<<"Path: "<<path<<endl;
	general_logs<<"First iter fast(only best V and best J considered): "<<fast_iter<<endl;
	general_logs<<"Min Likelihood threshold: "<<likelihood_threshold<<endl;
	general_logs<<"Viterbi like (only keeps the best scenario): "<<viterbi_like<<endl;
	general_logs<<"Proba threshold ratio: "<<proba_threshold_factor<<"\t#(ratio between best scenario and current scenario needed to explore/count the scenario)"<<endl;
	general_logs<<"Mean #errors threshold: "<<mean_number_seq_err_thresh<<"\t#Needs a very good reason to be set to another value than INFINITY"<<endl;

	//Get the total number of sequences to process
	const double total_number_seqs = sequences.size(); //Use a double for float division afterwards

	/*
	 * Get the list of fixed and inferred events and output them to the log file
	 * Do it in a scope so the variables will be destroyed
	 */
	{
		list<Rec_Event_name> fixed_events_list;
		list<Rec_Event_name> inferred_events_list;
		const list<shared_ptr<Rec_Event>> model_event_list = model_parms.get_event_list();
		for(list<shared_ptr<Rec_Event>>::const_iterator iter = model_event_list.begin() ; iter!=model_event_list.end() ; ++iter){
			if(not (*iter)->is_fixed()){
				inferred_events_list.emplace_back((*iter)->get_name());
			}
			else{
				fixed_events_list.emplace_back((*iter)->get_name());
			}
		}
		general_logs<<endl;
		general_logs<<"List of updated events: ";
		for(Rec_Event_name name : inferred_events_list){
			general_logs<<name<<"\t";
		}
		general_logs<<endl;
		general_logs<<"List of fixed events: ";
		for(Rec_Event_name name : fixed_events_list){
			general_logs<<name<<"\t";
		}
		general_logs<<endl;
		general_logs<<"Error model updated: "<<model_parms.get_err_rate_p()->is_updated()<<endl;
	}

	//Write initial condition to file
	this->model_marginals.write2txt(path+string("initial_marginals.txt"),this->model_parms);
	this->model_parms.write_model_parms(path+string("initial_model.txt"));

	/*
	 * First initialization creates file streams
	 * This is to make sure that the counter copies do not create new files each time
	 */
	for(map<size_t,shared_ptr<Counter>>::const_iterator iter = counters_list.begin() ; iter!=counters_list.end() ; ++iter){
		(*iter).second->initialize_counter(model_parms,model_marginals);
	}

/*
 * Reduction using OpenMP 4.0 standards
 *
	#pragma omp declare reduction(+:Model_marginals:omp_out+=omp_in) initializer(omp_priv = omp_orig.empty_copy())
	//Note: since Error_rate is an abstract class, the following is very dirty, need to find a better solution
	#pragma omp declare reduction(+:shared_ptr<Error_rate>:add_to_err_rate(omp_out,omp_in)) initializer(omp_priv = omp_orig->copy())
*/

	//Loop over iterations
	while(iteration_accomplished!=iterations){

		//double proba_threshold_factor;
		Model_marginals new_marginals = Model_marginals(model_parms);
		shared_ptr<Error_rate> error_rate_copy = model_parms.get_err_rate_p()->copy();


		//Initialize error rate copy
		error_rate_copy->initialize(model_parms.get_events_map());


		//Initialize counters for the log file
		size_t sequences_processed = 0;

		new_marginals.debug_marg_name = "new_marginals";

		const vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>* sequence_util_ptr;

		//Take only best alignments if fast_iter
		vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>> fast_iter_sequences;
		if(fast_iter && iteration_accomplished==0){
			fast_iter_sequences = sequences;
			for(unordered_map<Gene_class , vector<Alignment_data>>::const_iterator gc_align_iter = std::get<2>(sequences.at(0)).begin() ; gc_align_iter != std::get<2>(sequences.at(0)).end() ; ++gc_align_iter){
				if ((*gc_align_iter).first == D_gene)continue;
				fast_iter_sequences = get_best_aligns(fast_iter_sequences,(*gc_align_iter).first);
			}
			sequence_util_ptr = &fast_iter_sequences;
		}
		else{
			sequence_util_ptr = &sequences;
		}

		cerr<<"Performing Evaluate/Inference iteration "<<iteration_accomplished+1<<endl;

		/* omp parallel declaration using OpenMP 4.0 standards
		 * #pragma omp parallel for schedule(dynamic) reduction(+:error_rate_copy,new_marginals) firstprivate(model_queue,index_map,offset_map,model_marginals_copy,events_map , processed_events , safety_set , write_index_list) //num_threads(6)
		 */

		//Declare variables to use OpenMP 3.1 standards
		#pragma omp parallel shared(new_marginals,error_rate_copy,sequences_processed,sequence_util_ptr,sequences) firstprivate(model_queue,proba_threshold_factor ) //num_threads(1)
		{
			//Make single thread copies of objects for thread safety
			Model_Parms single_thread_model_parms (model_parms);
			unordered_map<Rec_Event_name,int> single_thread_index_map =model_marginals.get_index_map(model_parms,model_queue);
			Model_marginals single_thread_model_marginals (model_marginals);
			Model_marginals single_thread_marginals (single_thread_model_parms);
			shared_ptr<Error_rate> single_thread_err_rate = single_thread_model_parms.get_err_rate_p();
			unordered_map<tuple<Event_type,Gene_class,Seq_side>, shared_ptr<Rec_Event>> events_map = single_thread_model_parms.get_events_map();
			map<size_t,shared_ptr<Counter>> single_thread_counter_list;
			for(map<size_t,shared_ptr<Counter>>::const_iterator iter = this->counters_list.begin() ; iter != this->counters_list.end() ; ++iter){
				//Copy only relevant counters for this iteration
				if((not (*iter).second->is_last_iter_only()) or (iteration_accomplished == iterations-1)){
					single_thread_counter_list.emplace((*iter).first,(*iter).second->copy());
				}
			}

			single_thread_marginals.debug_marg_name = "single_thread_marginals";
			single_thread_model_marginals.debug_marg_name = "single thread model marginals";

			unordered_set<Rec_Event_name> init_processed_events;

			//Initialize Enum_fast_memory map and dual maps
			Safety_bool_map safety_set(3);
			Seq_type_str_p_map constructed_sequences(6);//6 is the number of outcomes for Seq_type
			Mismatch_vectors_map mismatches_lists(6);
			Seq_offsets_map seq_offsets(6,3);

			//Initialize downstream probas to 1
			Downstream_scenario_proba_bound_map downstream_proba_map(6);
			downstream_proba_map.init_first_layer(1.0);

			list<shared_ptr<Rec_Event>> events_list = single_thread_model_parms.get_event_list();
			Index_map index_mapp(events_list.size());

			//Initialize index_map
			for(list<shared_ptr<Rec_Event>>::iterator event_iter = events_list.begin() ; event_iter != events_list.end() ; ++event_iter){
				int event_index = (*event_iter)->get_event_identifier();
				index_mapp.request_memory_layer(event_index);
				index_mapp.set_value(event_index,single_thread_index_map.at((*event_iter)->get_name()) , 0);
				//TODO update proba bound

				//Get events probability upper bounds
				size_t event_size = single_thread_model_marginals.get_event_size((*event_iter) , single_thread_model_parms);

				(*event_iter)->set_event_marginal_size(event_size);
				(*event_iter)->set_crude_upper_bound_proba(single_thread_index_map.at((*event_iter)->get_name()) , event_size , single_thread_model_marginals.marginal_array_smart_p);

			}

			queue<shared_ptr<Rec_Event>> single_thread_model_queue = single_thread_model_parms.get_model_queue(); //single_thread_parms.get_model_queue();

			queue<shared_ptr<Rec_Event>> init_single_thread_model_queue = single_thread_model_queue;
			unordered_map<Rec_Event_name,vector<pair<shared_ptr<const Rec_Event>,int>>>single_thread_offset_map = model_marginals.get_offsets_map(model_parms,single_thread_model_queue);

			stack<shared_ptr<Rec_Event>> init_single_thread_stack;

			//Initialize events
			while(!init_single_thread_model_queue.empty()){
				shared_ptr<Rec_Event> first_init_event = init_single_thread_model_queue.front();
				init_single_thread_stack.push(first_init_event);
				init_single_thread_model_queue.pop();
				(*first_init_event).initialize_event(init_processed_events,events_map , single_thread_offset_map , downstream_proba_map , constructed_sequences,safety_set , single_thread_err_rate , mismatches_lists,seq_offsets , index_mapp);
				(*first_init_event).set_viterbi_run(viterbi_like);
			}

			/*
			 * Initialize the array of next event pointers
			 * This array replaces the formerly copied queue<shared_ptr<Rec_Event>> (was copied at each iterate_wrap_up call)
			 * Each event will access the pointer corresponding to its identifier address when calling iterate inside iterate_wrap_up
			 * The last event will point to null pointer enabling to call the error_rate
			 */
			shared_ptr<Next_event_ptr> next_event_ptr_arr (new Next_event_ptr[single_thread_model_parms.get_event_list().size()]);
			init_single_thread_model_queue = single_thread_model_queue;
			while(!init_single_thread_model_queue.empty()){
				shared_ptr<Rec_Event> first_init_event = init_single_thread_model_queue.front();
				init_single_thread_model_queue.pop();
				if(!init_single_thread_model_queue.empty()){
					next_event_ptr_arr.get()[first_init_event->get_event_identifier()] = init_single_thread_model_queue.front().get();
				}
				else{
					//This is the last event thus we emplace a shared null pointer
					next_event_ptr_arr.get()[first_init_event->get_event_identifier()] = NULL; //Next_event_ptr(NULL);
				}
			}


			//Initialize error rate
			single_thread_err_rate->initialize(events_map);
			single_thread_err_rate->set_viterbi_run(viterbi_like);


			//Initialize Counters
			for(map<size_t,shared_ptr<Counter>>::iterator iter = single_thread_counter_list.begin() ; iter!=single_thread_counter_list.end() ; ++iter){
				(*iter).second->initialize_counter(single_thread_model_parms , single_thread_marginals);
			}
			#pragma omp single nowait
			{
				cerr<<"Initializing probability bounds..."<<endl;
			}
			//Compute upper proba bounds for downstream scenarios for each event
			double downstream_proba_bound = 1 ;
			forward_list<double*> updated_proba_list ;
			while(!init_single_thread_stack.empty()){
				shared_ptr<Rec_Event> last_proba_init_event = init_single_thread_stack.top();
				queue<shared_ptr<Rec_Event>> tmp_init_proba_single_thread_model_queue = single_thread_model_queue;
				init_single_thread_stack.pop();
				while(tmp_init_proba_single_thread_model_queue.front()!=last_proba_init_event){
					tmp_init_proba_single_thread_model_queue.pop();
				}
				tmp_init_proba_single_thread_model_queue.pop();
				last_proba_init_event->initialize_crude_scenario_proba_bound(downstream_proba_bound , updated_proba_list , events_map);

				last_proba_init_event->initialize_Len_proba_bound(tmp_init_proba_single_thread_model_queue,single_thread_model_marginals.marginal_array_smart_p,index_mapp);
				/*#pragma omp single nowait
				{
					cerr<<last_proba_init_event->get_name()<<" initialized"<<endl;
				}*/
			}
			#pragma omp single nowait
			{
				cerr<<"Initialization of probability bounds over."<<endl;
			}

			//Now let all the events in the need of it get their own updated copy of the marginals
			init_single_thread_model_queue = single_thread_model_queue;
			while(!init_single_thread_model_queue.empty()){
				init_single_thread_model_queue.front()->update_event_internal_probas(single_thread_model_marginals.marginal_array_smart_p,index_map);
				init_single_thread_model_queue.pop();
			}

			chrono::system_clock::time_point single_seq_begin;
			chrono::duration<double> seq_time;




			//Loop over sequences in parallel, using the number of threads declared previously when declaring the parallel section
			//Use dynamic scheduling to avoid loss of time due to synchronization

			#pragma omp for schedule(dynamic) nowait
			for(vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>::const_iterator seq_it = (*sequence_util_ptr).begin() ; seq_it < (*sequence_util_ptr).end() ; ++seq_it){

				single_seq_begin = chrono::system_clock::now();

				//Make a copy of the queue that can be modified in iterate
				queue<shared_ptr<Rec_Event>> model_queue_copy(single_thread_model_queue);

				//Get the first event from the queue
				shared_ptr<Rec_Event> first_event = model_queue_copy.front();
				model_queue_copy.pop();


				//Initialize single seq marginals
				Model_marginals single_seq_marginals = single_thread_model_marginals.empty_copy();
				double init_proba = 1;
				//double init_tmp_err_w_proba = 1;
				double max_proba_scenario = likelihood_threshold/proba_threshold_factor;

				Int_Str int_sequence = nt2int(get<1>(*seq_it));

				//cout<<int_sequence<<endl;

				single_seq_marginals.debug_marg_name = "single_seq_marginals";

				/*
				 * Call iterate on the first event
				 * The method will be called recursively for each event, this is equivalent to a nested loop and enumerates all possible scenarios
				 * The weight of each recombination scenario is added to the single_seq_marginals on the fly
				 */
				try{

					first_event->iterate(init_proba , downstream_proba_map , get<1>(*seq_it) , int_sequence , index_mapp , single_thread_offset_map , next_event_ptr_arr , single_seq_marginals.marginal_array_smart_p , single_thread_model_marginals.marginal_array_smart_p , get<2>(*seq_it) , constructed_sequences , seq_offsets , single_thread_err_rate , single_thread_counter_list , events_map , safety_set , mismatches_lists , max_proba_scenario , proba_threshold_factor);

				}

				catch(exception& except){
					general_logs<<"Exception caught calling iterate() on sequence:"<<endl;
					general_logs<<get<1>(*seq_it)<<" with index "<<get<0>(*seq_it)<<endl;
					general_logs<<"Exception caught after "<<single_thread_err_rate->debug_number_scenarios<<" scenarios explored"<<endl;
					general_logs<<endl;
					general_logs<<"Throwing exception now..."<<endl<<endl;
					general_logs<<except.what()<<endl;
					throw;
				}



				//Normalize the weights on the single_seq_marginal so that each sequence has the same weight when merged to the single_thread_marginals
				single_thread_err_rate->norm_weights_by_seq_likelihood(single_seq_marginals.marginal_array_smart_p,single_seq_marginals.get_length());
				seq_time = chrono::system_clock::now() - single_seq_begin;
				#pragma omp critical(dump_seq_info)
				{
					++sequences_processed;
					//Output useful infos in the log file
					//log_file<<iteration_accomplished<<";"<<sequences_processed<<";"<<(*seq_it).first<<";"<<(*seq_it).second.at(V_gene).size()<<";"<<(*seq_it).second.at(D_gene).size()<<";"<<(*seq_it).second.at(J_gene).size()<<";"<<single_thread_err_rate->get_seq_probability()<<";"<<single_thread_err_rate->get_seq_likelihood()<<";"<<single_thread_err_rate->debug_number_scenarios<<";"<<max_proba_scenario<<endl;
					log_file<<iteration_accomplished<<";"<<sequences_processed<<";"<<get<0>(*seq_it)<<";"<<get<1>(*seq_it)<<";"<<get<2>(*seq_it).at(V_gene).size()<<";"<<get<2>(*seq_it).at(J_gene).size()<<";"<<single_thread_err_rate->get_seq_likelihood()<<";"<<single_thread_err_rate->get_seq_mean_error_number()<<";"<<single_thread_err_rate->debug_number_scenarios<<";"<<max_proba_scenario<<";"<<seq_time.count()<<endl;
				}
				for(map<size_t,shared_ptr<Counter>>::iterator iter = single_thread_counter_list.begin() ; iter!=single_thread_counter_list.end() ; ++iter){
					iter->second->count_sequence(single_thread_err_rate->get_seq_likelihood() , single_seq_marginals , single_thread_model_parms);
					#pragma omp critical(dump_counters)
					{
						(*iter).second->dump_sequence_data(get<0>(*seq_it) , iteration_accomplished);
					}
				}


				if(single_thread_err_rate->get_seq_mean_error_number()<=mean_number_seq_err_thresh){
					//Add weighed errors to the normalized error counter
					single_thread_err_rate->add_to_norm_counter();

					//Add the single_seq_marginals to the single thread marginals
					single_thread_marginals+=single_seq_marginals;
				}
				else{
					//Erase seq specific counters so that it won't contribute to the error rate
					single_thread_err_rate->clean_seq_counters();
				}

				#pragma omp critical (update_progress_bar)
				{
					if(sequences_processed%100 == 0){
						//Output current progress to cerr
						show_progress_bar(cerr,sequences_processed/total_number_seqs, "Iteration "+ to_string(iteration_accomplished+1), 50);
					}
				}

			}


			//Merge single thread error_rates and marginals
			#pragma omp critical(merge_marginals_and_er)
			{
				new_marginals+=single_thread_marginals;
				add_to_err_rate(error_rate_copy.get(),single_thread_err_rate.get());
				for(map<size_t,shared_ptr<Counter>>::iterator iter = single_thread_counter_list.begin() ; iter!=single_thread_counter_list.end() ; ++iter){
					counters_list.at((*iter).first)->add_to_counter((*iter).second);
				}
			}


		}


		for(map<size_t,shared_ptr<Counter>>::const_iterator iter = counters_list.begin() ; iter!=counters_list.end() ; ++iter){
			(*iter).second->dump_data_summary(iteration_accomplished);
		}

		likelihood_file<<iteration_accomplished+1<<";"<<error_rate_copy->get_model_likelihood()/error_rate_copy->get_number_non_zero_likelihood_seqs()<<";"<<error_rate_copy->get_number_non_zero_likelihood_seqs()<<endl;
		error_rate_copy->update();
		this->model_parms.set_error_ratep(error_rate_copy);
		new_marginals.normalize(inv_offset_map , index_map , model_queue);
		new_marginals.copy_fixed_events_marginals(this->model_marginals,this->model_parms,index_map);
		this->model_marginals = new_marginals;
		++iteration_accomplished;

		this->model_marginals.write2txt(path+string("iteration_")+to_string(iteration_accomplished)+string(".txt"),this->model_parms);
		this->model_parms.write_model_parms(path+string("iteration_")+to_string(iteration_accomplished)+string("_parms.txt"));

		//Close current iteration progress bar
		close_progress_bar(cerr, "Iteration " + to_string(iteration_accomplished), 50);

	}
	//Create a copy of the last iteration results with identifiable name
	this->model_marginals.write2txt(path+string("final_marginals.txt"),this->model_parms);
	this->model_parms.write_model_parms(path+string("final_parms.txt"));

	return 0;
}
/**
 * \deprecated This function used to store generated sequences in memory, and quickly overloaded it for large number of generated sequences.
 */
forward_list<pair<string,queue<queue<int>>>> GenModel::generate_sequences(int number_seq , bool generate_errors){

	queue<shared_ptr<Rec_Event>> model_queue = this->model_parms.get_model_queue();
	unordered_map<Rec_Event_name,int> index_map = this->model_marginals.get_index_map(this->model_parms,model_queue);
	unordered_map<Rec_Event_name,vector<pair<shared_ptr<const Rec_Event> , int>>> offset_map = this->model_marginals.get_offsets_map(this->model_parms,model_queue);

	//Create seed for random generator
	//create a seed from timer
	typedef std::chrono::high_resolution_clock myclock;
	myclock::time_point time = myclock::now();
	myclock::duration dur = myclock::time_point::max() - time;

	//Get a random seed
	uint64_t random_seed = draw_random_64bits_seed();
	//Instantiate random number generator
	mt19937_64 generator =  mt19937_64(random_seed);
	forward_list<pair<string,queue<queue<int>>>> sequence_list =  forward_list<pair<string,queue<queue<int>>>>();

	for(int seq = 0 ; seq != number_seq ; ++seq){
		pair<string,queue<queue<int>>> sequence = this->generate_unique_sequence(model_queue , index_map ,offset_map , generator);
		if(generate_errors){
			sequence.second.push(this->model_parms.get_err_rate_p()->generate_errors(sequence.first,generator));
		}
		sequence_list.push_front(sequence);
		if(seq%1000 == 0){
			//Output current progress to cerr
			show_progress_bar(cerr,seq/(double) number_seq, "Sequence generation", 50);
		}
	}
	close_progress_bar(cerr, "Sequence generation", 50);

	return sequence_list;

}
/*
 * Generate sequences in a memory efficient way
 */
void GenModel::generate_sequences(int number_seq,bool generate_errors , string filename_ind_seq , string filename_ind_real,list<pair<gen_seq_trans,shared_ptr<void>>> transform_func_and_data /*= list<pair<gen_seq_trans,shared_ptr<void>>>()*/ , bool output_only_func /*= false*/, int seed /* =-1*/ ){
	ofstream outfile_ind_seq;
	ofstream outfile_ind_real;
	if(not output_only_func){
		outfile_ind_seq.open(filename_ind_seq);
		outfile_ind_real.open(filename_ind_real);
	}
	string folder_path = filename_ind_seq.substr(0,filename_ind_seq.rfind("/")+1); //Get the file path
	ofstream generation_infos_file(folder_path + "generation_info.out",fstream::out | fstream::app); //Opens the file in append mode




	//Create a header for the files
	queue<shared_ptr<Rec_Event>> model_queue = this->model_parms.get_model_queue();
	if(not output_only_func){
		outfile_ind_seq<<"seq_index;nt_sequence"<<endl;
		outfile_ind_real<<"seq_index";
		while(!model_queue.empty()){
			outfile_ind_real<<";"<<model_queue.front()->get_name();
			model_queue.pop();
		}
		outfile_ind_real<<";Errors"<<endl;
	}
	model_queue = this->model_parms.get_model_queue();
	unordered_map<Rec_Event_name,int> index_map = this->model_marginals.get_index_map(this->model_parms,model_queue);
	unordered_map<Rec_Event_name,vector<pair<shared_ptr<const Rec_Event> , int>>> offset_map = this->model_marginals.get_offsets_map(this->model_parms,model_queue);

	//Create seed for random generator
	//create a seed from timer if no seed was provided
	uint64_t random_seed;
	if(seed<0){
		//Get a random seed
		random_seed = draw_random_64bits_seed();
	}else{
		random_seed = seed;
	}
	clog<<"Seed: "<<random_seed<<endl;
	//Instantiate random number generator
	mt19937_64 generator =  mt19937_64(random_seed);


	chrono::system_clock::time_point begin_time = chrono::system_clock::now();
	std::time_t tt;
	tt = chrono::system_clock::to_time_t ( begin_time );

	generation_infos_file<<endl<<"================================================================"<<endl;
	generation_infos_file<<"Generated sequences in file: "<<filename_ind_seq<<endl;
	generation_infos_file<<"Generated sequences realizations in file: "<<filename_ind_real<<endl;
	generation_infos_file<<"Date: "<< ctime(&tt)<<endl;
	generation_infos_file<<"Number of sequences = "<<number_seq<<endl;
	generation_infos_file<<"Generated with errors = "<<generate_errors<<endl;
	generation_infos_file<<"Seed  = "<<random_seed<<endl;

	//Update events internal probas (e.g for dinucleotide ambiguous nucleotides)
	queue<shared_ptr<Rec_Event>> model_queue_copy = model_queue;
	while(not model_queue_copy.empty()){
		model_queue_copy.front()->update_event_internal_probas(this->model_marginals.marginal_array_smart_p , index_map);
		model_queue_copy.pop();
	}

	for(size_t seq = 0 ; seq != number_seq ; ++seq){
		pair<string,queue<queue<int>>> sequence = this->generate_unique_sequence(model_queue , index_map ,offset_map , generator,false);
		if(generate_errors){
			sequence.second.push(this->model_parms.get_err_rate_p()->generate_errors(sequence.first,generator));
		}

		for(pair<gen_seq_trans,shared_ptr<void>> func_data_pair : transform_func_and_data){
			func_data_pair.first(seq,sequence,func_data_pair.second);
		}

		if(not output_only_func){
			outfile_ind_seq<<seq<<";"<<sequence.first<<endl;
			outfile_ind_real<<seq;
			queue<queue<int>>& realizations = sequence.second;
			while(!realizations.empty()){
				outfile_ind_real<<";";
				queue<int> event_real = realizations.front();
				outfile_ind_real<<"(";
				while(!event_real.empty()){
					outfile_ind_real<<event_real.front();
					event_real.pop();
					if(!event_real.empty()){
						outfile_ind_real<<",";
					}
				}
				outfile_ind_real<<")";
				realizations.pop();
			}
			outfile_ind_real<<endl;
		}

		if(seq%1000 == 0){
			//Output current progress to cerr
			show_progress_bar(cerr,seq/(double) number_seq, "Sequence generation", 50);
		}


	}
	close_progress_bar(cerr, "Sequence generation", 50);
	return;
}

pair<string,queue<queue<int>>> GenModel::generate_unique_sequence(queue<shared_ptr<Rec_Event>> model_queue , unordered_map<Rec_Event_name,int> index_map , const unordered_map<Rec_Event_name,vector<pair<shared_ptr<const Rec_Event> , int>>>& offset_map , mt19937_64& generator , bool update_event_internal_proba /*= true*/ ){
	if(update_event_internal_proba){
		queue<shared_ptr<Rec_Event>> model_queue_copy = model_queue;
		while(not model_queue_copy.empty()){
			model_queue_copy.front()->update_event_internal_probas(this->model_marginals.marginal_array_smart_p , index_map);
			model_queue_copy.pop();
		}
	}


	unordered_map<Seq_type,string>* constructed_sequences_p = new unordered_map<Seq_type,string>;
	unordered_map<Seq_type,string> constructed_sequences = *constructed_sequences_p;
	queue<queue<int>> realizations ;
	while(! model_queue.empty()){
		realizations.push(model_queue.front()->draw_random_realization((this->model_marginals.marginal_array_smart_p) , index_map , offset_map , constructed_sequences ,  generator));
		model_queue.pop();
	}
	//CAT strings
	string reconstructed_seq = constructed_sequences[V_gene_seq] + constructed_sequences[VJ_ins_seq] + constructed_sequences[VD_ins_seq] + constructed_sequences[D_gene_seq] + constructed_sequences[DJ_ins_seq] + constructed_sequences[J_gene_seq];
	delete constructed_sequences_p;
	return make_pair(reconstructed_seq,realizations);
}

void GenModel::write_seq2txt(string filename , forward_list<string> sequences){
	ofstream outfile (filename);
	for(forward_list<string>::const_iterator seq = sequences.begin() ; seq != sequences.end() ; ++seq){
		outfile<<(*seq)<<endl;
	}
}

void GenModel::write_seq_real2txt(string filename_ind_seq , string filename_ind_real , forward_list<pair<string,queue<queue<int>>>> seq_and_realizations){
	ofstream outfile_ind_seq(filename_ind_seq);
	ofstream outfile_ind_real(filename_ind_real);

	//Create a header for the files
	outfile_ind_seq<<"seq_index;nt_sequence"<<endl;
	queue<shared_ptr<Rec_Event>> model_queue = this->model_parms.get_model_queue();
	outfile_ind_real<<"Index";
	while(!model_queue.empty()){
		outfile_ind_real<<";"<<model_queue.front()->get_name();
		model_queue.pop();
	}
	outfile_ind_real<<endl;

	size_t index = 0;
	for(forward_list<pair<string,queue<queue<int>>>>::const_iterator iter = seq_and_realizations.begin() ; iter != seq_and_realizations.end() ; iter++){
		outfile_ind_seq<<index<<";"<<(*iter).first<<endl;
		outfile_ind_real<<index;
		queue<queue<int>> realizations = (*iter).second;
		while(!realizations.empty()){
			outfile_ind_real<<";";
			queue<int> event_real = realizations.front();
			outfile_ind_real<<"(";
			while(!event_real.empty()){
				outfile_ind_real<<event_real.front();
				event_real.pop();
				if(!event_real.empty()){
					outfile_ind_real<<",";
				}
			}
			outfile_ind_real<<")";
			realizations.pop();
		}
		outfile_ind_real<<endl;
		index++;
	}

}

/*
 * Extract the best alignment for each sequence for a given gene class (used for the fast iter)
 */
vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>> get_best_aligns (const vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>& all_aligns, Gene_class gc){

	vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>> all_aligns_copy (all_aligns);
	for(vector<tuple<int,string,unordered_map<Gene_class , vector<Alignment_data>>>>::iterator seq_iter = all_aligns_copy.begin() ; seq_iter!=all_aligns_copy.end() ; ++seq_iter){
		vector<Alignment_data>& align_vect = get<2>((*seq_iter)).at(gc); //TODO add exception

		//Get align best score
		double best_score = -1;
		for(vector<Alignment_data>::const_iterator align_iter = align_vect.begin() ; align_iter!=align_vect.end() ; ++align_iter){
			if((*align_iter).score>best_score){best_score=(*align_iter).score;}
		}

		vector<Alignment_data> new_align_vect ;
		for(vector<Alignment_data>::const_iterator align_iter = align_vect.begin() ; align_iter!=align_vect.end() ; ++align_iter){
			if((*align_iter).score==best_score){new_align_vect.push_back((*align_iter));}
		}

		align_vect = new_align_vect;
	}

	return all_aligns_copy;
}
/**
 * FIXME for now the handling of non given anchors is very bad
 */
void output_CDR3_gen_data(size_t seq_index, std::pair<std::string , std::queue<std::queue<int>>> seq_and_real ,std::shared_ptr<void> func_data){
	gen_CDR3_data* func_data_cast = static_cast<gen_CDR3_data*>(func_data.get());


	tuple<string,size_t,size_t,string>* v_gene_anchors;
	tuple<string,size_t,size_t,string>* j_gene_anchors;

	size_t i = 0;
	while( (i!=max(func_data_cast->v_event_queue_position,func_data_cast->j_event_queue_position)+1)
			and (not seq_and_real.second.empty())){
		if(i== func_data_cast->v_event_queue_position){
			v_gene_anchors = &func_data_cast->v_anchors.at(seq_and_real.second.front().front());
			//There should be only one realization for v gene choice
		}
		else if(i== func_data_cast->j_event_queue_position){
			j_gene_anchors = &func_data_cast->j_anchors.at(seq_and_real.second.front().front());
		}
		seq_and_real.second.pop();
		++i;
	}

	//Compute the index of the last letter of the J anchor
	size_t tmp_index = seq_and_real.first.size() - get<2>(*j_gene_anchors) + get<1>(*j_gene_anchors) +2;
	size_t tmp_v_index = get<1>(*v_gene_anchors);
	string nt_cdr3_seq = seq_and_real.first.substr(tmp_v_index, tmp_index - tmp_v_index +1);


/*	if( (nt_cdr3_seq.substr(0,3) == get<3>(*v_gene_anchors)) and (nt_cdr3_seq.substr(nt_cdr3_seq.size()-3,3) == get<3>(*j_gene_anchors))){
		func_data_cast->output_file<<nt_cdr3_seq<<",,"<<true<<",";
		if(nt_cdr3_seq.size()%3==0){
			func_data_cast->output_file<<true<<","<<endl;
		}
		else{
			func_data_cast->output_file<<false<<","<<endl;
		}
	}
	else{
		func_data_cast->output_file<<",,,"<<false<<","<<false<<","<<false<<endl;
	}
*/
	*func_data_cast->output_stream<<seq_index;
	if(func_data_cast->output_nt_CDR3){
		*func_data_cast->output_stream<<","<<nt_cdr3_seq;
	}
	if(func_data_cast->output_anchors_found){
		bool anchors_found = (nt_cdr3_seq.substr(0,3) == get<3>(*v_gene_anchors)) and (nt_cdr3_seq.substr(nt_cdr3_seq.size()-3,3) == get<3>(*j_gene_anchors));
		*func_data_cast->output_stream<<","<<anchors_found;
	}
	if(func_data_cast->output_inframe){
		bool is_inframe = nt_cdr3_seq.size()%3==0;
		*func_data_cast->output_stream<<","<<is_inframe;
	}
	if(func_data_cast->output_aa_CDR3){
		//FIXME
		*func_data_cast->output_stream<<","<<"";
	}
	if(func_data_cast->output_productive){
		//FIXME
		*func_data_cast->output_stream<<","<<"";
	}
	*func_data_cast->output_stream<<endl;
}
