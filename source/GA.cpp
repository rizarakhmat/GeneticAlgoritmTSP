#include "dependencies.h"

GA::GA(City& c, double r, int w, int n, int p, int i, double cp) : city(c){
    resistence = r;
    nw = w;
    n_nodes = n;
    pop_size = p;
    iterations = i;
    crossover_prob = cp;
}

#ifdef GRAPH
long GA::evolution_seq(Draw draw){
#else
long GA::evolution_seq(){
#endif

    //std::cout << "Population :" << sizeof(int)*n_nodes*pop_size/1000000. << "MB, Affinities: " << sizeof(double)*pop_size/1000000. << "MB" << std::endl;
    MyRandom myrandom(n_nodes, pop_size);
    Population population(pop_size, n_nodes, myrandom);
    population.generate_population();
    population.calculate_affinities(city);

    std::vector<std::vector<int>> new_population = std::vector<std::vector<int>>(pop_size);	
    std::vector<double> new_affinities = std::vector<double>(pop_size);

    double best_score = DBL_MAX;
    std::vector<int> best_path;

    double sum=0;
    double score=0;
    int i=0;

    long time;
    
    { utimer ts{"Total time", &time};
    for(int c=0; c<iterations; c++){
	sum=0;
	for(i=0; i<pop_size; i++){
	    new_population[i] = population.crossover(population.pick_candidate(), population.pick_candidate(), resistence, city, crossover_prob);
	    score = city.path_length(new_population[i]); // evaluation/fitness
	    if(score<best_score){
		best_score = score;
		best_path = new_population[i];
		#ifdef GRAPH
	        draw.clear();
	        draw.print_city(city.x, city.y, city.adj);
	        draw.print_best_one(best_path, city.x, city.y);
		#endif
	    }
	    new_affinities[i] = 1/(score+1); // lower score is better affinity
	    sum += new_affinities[i];
	}
	for(i=0; i<pop_size; i++){
	    new_affinities[i] = new_affinities[i]/sum; // normalization
    	}
	population.population = new_population;
	population.affinities = new_affinities;
    }
    } //utimer
    //std::cout << "Best length: " << best_score << std::endl;
    return time;
}

// --------------------------------------------THREAD-------------------------------------------
long GA::evolution_thread(){

    int pop_thread = pop_size/nw; 
    std::vector<std::thread> threads;

    //std::cout << "Population :" << sizeof(int)*n_nodes*pop_thread/1000000. << "MB, Affinities: " << sizeof(double)*pop_thread/1000000. << "MB" << std::endl;

    std::vector<std::tuple<std::vector<int>, double>> results = std::vector<std::tuple<std::vector<int>, double>>(nw);

    // define threads behaviour
    auto myJob = [this, pop_thread, &results](int k) {

        MyRandom myrandom(n_nodes, pop_thread);
	Population population(pop_thread, n_nodes, myrandom);
	population.generate_population();
	population.calculate_affinities(city);

	std::vector<std::vector<int>> new_population = std::vector<std::vector<int>>(pop_thread);	
	std::vector<double> new_affinities = std::vector<double>(pop_thread);

	double best_score = DBL_MAX;
	std::vector<int> best_path;

	//variables of the cycle
	double sum=0;
	int i=0;
	double score=0;

	for(int l=0; l<iterations; l++){
	    sum=0;

	    // create new population and calculate score, then set it as population actual attribute
	    for(i=0; i<pop_thread; i++){
	        new_population[i] = population.crossover(population.pick_candidate(), population.pick_candidate(), resistence, city, crossover_prob);
	        score = city.path_length(new_population[i]);
	        if(score<best_score){
		    best_score = score;
		    best_path = new_population[i];
	        }
	        new_affinities[i] = 1/(score+1);
	        sum += new_affinities[i];

	    }
	    // normalize
	    for(i=0; i<pop_thread; i++){
	        new_affinities[i] = new_affinities[i]/sum;
    	    }
	    // evolve
	    population.population = new_population;
	    population.affinities = new_affinities;
        }
        results[k] = std::tuple<std::vector<int>, double>(best_path, best_score);
    };

    // start threads
    long time;
    double best_length = DBL_MAX;
    { utimer ts{"Total time", &time};
    for(int t=0; t<nw; t++)
        threads.push_back(std::thread(myJob, t));
    for(int t=0; t<nw; t++)
        threads[t].join();
    for(int t=0; t<nw; t++){
	if(std::get<1>(results[t]) < best_length)
	    best_length = std::get<1>(results[t]);
    }
    }//timer
    //std::cout << "Best length: " << best_length << std::endl;
    return time;
}

// *********************************************** FASTFLOW **********************************************************

struct Evolution: ff_node_t<std::tuple<std::vector<int>, double>> {

Evolution(int n_nodes, int pop_ff, int iterations, City city, double resistence, double crossover_prob): n_nodes(n_nodes), pop_ff(pop_ff), iterations(iterations), city(city), resistence(resistence), crossover_prob(crossover_prob) {}

    int n_nodes;
    int pop_ff;
    int iterations;
    City city;
    double resistence;
    double crossover_prob;

    std::tuple<std::vector<int>, double>* svc(std::tuple<std::vector<int>, double>*) {

	MyRandom myrandom(n_nodes, pop_ff);
	Population population(pop_ff, n_nodes, myrandom);
	population.generate_population();
	population.calculate_affinities(city);

	std::vector<std::vector<int>> new_population = std::vector<std::vector<int>>(pop_ff);	
	std::vector<double> new_affinities = std::vector<double>(pop_ff);

	double best_score = DBL_MAX;
	std::vector<int> best_path;

	//variables of the cycle
	double sum=0;
	int i=0;
	double score=0;

	for(int l=0; l<iterations; l++){
	    sum=0;

	    // create new population and calculate score, then set it as population actual attribute
	    for(i=0; i<pop_ff; i++){
		new_population[i] = population.crossover(population.pick_candidate(), population.pick_candidate(), resistence, city, crossover_prob);
		score = city.path_length(new_population[i]);
		if(score<best_score){
		    best_score = score;
		    best_path = new_population[i];
		}
		new_affinities[i] = 1/(score+1);
		sum += new_affinities[i];
	    }
	    // normalize
	    for(i=0; i<pop_ff; i++){
		new_affinities[i] = new_affinities[i]/sum;
	    }
	    // evolve
	    population.population = new_population;
	    population.affinities = new_affinities;
	} // for
	std::tuple<std::vector<int>, double>* res = new std::tuple<std::vector<int>, double>(best_path, best_score);
	return res;		
    } // svc
}; //struct

struct BestSelecter: ff_node_t<std::tuple<std::vector<int>, double>> {

    std::vector<int>* best_path = NULL;
    double best_length = DBL_MAX;

    std::tuple<std::vector<int>, double>* svc(std::tuple<std::vector<int>, double>* res) {
	if(best_path == NULL){
	    best_path = &std::get<0>(*res);
	    best_length = std::get<1>(*res);
	}
	else
	if(std::get<1>(*res) < best_length){
	   best_path = &std::get<0>(*res);
	   best_length = std::get<1>(*res);
	}
	return GO_ON;
    }
};

long GA::evolution_ff(){

    //std::cout << "Population :" << sizeof(int)*n_nodes*(pop_size/nw)/1000000. << "MB, Affinities: " << sizeof(double)*(pop_size/nw)/1000000. << "MB" << std::endl;

    int pop_ff = pop_size/nw;

    ff_Farm<float> farm( [&]() {
        std::vector<std::unique_ptr<ff_node> > work;
        for(int i=0;i<nw;++i)
	    work.push_back(make_unique<Evolution>(n_nodes, pop_ff, iterations, city, resistence, crossover_prob));
        return work;
    } () );

    BestSelecter bs;
    farm.add_collector(bs);
    
    long time;
    { utimer ts{"Total time", &time};
    farm.run_and_wait_end();
    }
    //std::cout << "Best length " << bs.best_length << std::endl;
    return time;
}

