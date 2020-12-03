/*
 * Dinuclmarkov.h
 *
 *  Created on: Mar 4, 2015
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
 *      Please note that although this class inherits Rec_event properties, it is not currently designed to be parent
 *      of any other event in the model_parms graph
 */

#ifndef DINUCLMARKOV_H_
#define DINUCLMARKOV_H_

#include "Rec_Event.h"
#include "Utils.h"
#include <forward_list>
#include <unordered_map>
#include <string>
#include <list>
#include <queue>
#include <utility>
#include "Errorrate.h"
#include <random>

/**
 * \class Dinucl_markov Dinucl_markov.h
 * \brief Dinucleotide insertion Markov model.
 * \author Q.Marcou
 * \version 1.0
 *
 * Models a Markov chain dictating the identity of inserted nucleotides in the inserted region.
 * We assume a low error frequency and almost flat dinucleotide model regime such that we use an euristic to extract the most likely realization.
 * This choice has been made because the full handling through a forward algorithm would not be able to cope with e.g context dependent errors.
 *
 * By construction the Insertion event must have been explored first
 */
class Dinucl_markov: public Rec_Event {
public:
	//Constructors
	Dinucl_markov(Gene_class);//TODO should be scalable on one side easily (mono di tri quadri nucl)
	//Destructor
	virtual ~Dinucl_markov();

	//Accessors
	std::shared_ptr<Rec_Event> copy();
	int size() const;


	inline void iterate(double& , Downstream_scenario_proba_bound_map& , const std::string& , const Int_Str& , Index_map& , const std::unordered_map<Rec_Event_name,std::vector<std::pair<std::shared_ptr<const Rec_Event>,int>>>& , std::shared_ptr<Next_event_ptr>& , Marginal_array_p& , const Marginal_array_p& , const std::unordered_map<Gene_class , std::vector<Alignment_data>>& , Seq_type_str_p_map& , Seq_offsets_map& , std::shared_ptr<Error_rate>& , std::map<size_t,std::shared_ptr<Counter>>& , const std::unordered_map<std::tuple<Event_type,Gene_class,Seq_side>, std::shared_ptr<Rec_Event>> & , Safety_bool_map& , Mismatch_vectors_map& , double& , double&);
	void add_realization(int);
	std::queue<int> draw_random_realization( const Marginal_array_p& , std::unordered_map<Rec_Event_name,int>& , const std::unordered_map<Rec_Event_name,std::vector<std::pair<std::shared_ptr<const Rec_Event>,int>>>& , std::unordered_map<Seq_type , std::string>& , std::mt19937_64&)const;
	void write2txt(std::ofstream&);
	void ind_normalize(Marginal_array_p&,size_t) const;
	void initialize_event( std::unordered_set<Rec_Event_name>& , const std::unordered_map<std::tuple<Event_type,Gene_class,Seq_side>, std::shared_ptr<Rec_Event>>& , const std::unordered_map<Rec_Event_name,std::vector<std::pair<std::shared_ptr<const Rec_Event>,int>>>& , Downstream_scenario_proba_bound_map& , Seq_type_str_p_map& , Safety_bool_map& , std::shared_ptr<Error_rate> , Mismatch_vectors_map&,Seq_offsets_map&,Index_map&);
	void add_to_marginals(long double , Marginal_array_p&) const;
	void update_event_internal_probas(const Marginal_array_p& , const std::unordered_map<Rec_Event_name,int>&);


	double* get_updated_ptr();
	void initialize_crude_scenario_proba_bound(double& , std::forward_list<double*>& , const std::unordered_map<std::tuple<Event_type,Gene_class,Seq_side>, std::shared_ptr<Rec_Event>>&);

	//Proba bound related computation methods
	bool has_effect_on(Seq_type) const;
	void iterate_initialize_Len_proba( Seq_type considered_junction ,  std::map<int,double>& length_best_proba_map ,  std::queue<std::shared_ptr<Rec_Event>>& model_queue , double& scenario_proba , const Marginal_array_p& model_parameters_point , Index_map& base_index_map , Seq_type_str_p_map& constructed_sequences , int& seq_len ) const ;
	void initialize_Len_proba_bound(std::queue<std::shared_ptr<Rec_Event>>& model_queue , const Marginal_array_p& model_parameters_point , Index_map& base_index_map );


private:

	double* updated_upper_bound_proba; //This points to a double modified by the Insertion event given the number of insertion
	Matrix<double> dinuc_proba_matrix;

	int total_nucl_count;
	//Int_Str vd_seq;//&
	int max_vd_ins;
	int* vd_realizations_indices;
	size_t vd_seq_size;
	//Int_Str vj_seq;//&
	int max_vj_ins;
	int* vj_realizations_indices;
	size_t vj_seq_size;
	//Int_Str dj_seq;//&
	int max_dj_ins;
	int* dj_realizations_indices;
	size_t dj_seq_size;

	Int_Str previous_seq;//&
	size_t previous_seq_size;
	int previous_nt_str;
	Int_Str data_seq_substr;

	mutable int base_index;
	int unmutable_base_index;
	double new_scenario_proba;
	double proba_contribution;
	mutable bool correct_class;

	int memory_layer_proba_map_junction_1;
	int memory_layer_proba_map_junction_2;


	//std::pair<Seq_type,Seq_side> v_5_pair = std::make_pair (V_gene_seq,Five_prime);
	//std::pair<Seq_type,Seq_side> j_5_pair = std::make_pair (J_gene_seq,Five_prime);

	//Iterate common
	int first_nt_index;
	int sec_nt_index;
	int offset;
	int realization_final_index;



	inline void iterate_common( int* , int& , Int_Str& , const Marginal_array_p&);
	inline std::queue<int> draw_random_common(const std::string& , std::string& , const Marginal_array_p& , int , std::uniform_real_distribution<double>& , std::mt19937_64&) const;
	inline double compute_nt_freq( int , const Marginal_array_p&) const;

};

#endif /* DINUCLMARKOV_H_ */
