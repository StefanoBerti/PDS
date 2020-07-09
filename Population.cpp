#include "dependencies.h"


#include <mutex>

using namespace ff;

Population::Population(int p, int n){
	pop_size = p;
	n_nodes = n;
	population = std::vector<std::vector<int>>(pop_size);
	affinities = std::vector<double>(pop_size);
	min_length = DBL_MAX;
}

void Population::generate_population(){
    for(int k=0; k<pop_size; k++){
	std::vector<int> path = std::vector<int>(n_nodes);
	    for(int i=0; i<n_nodes; i++) // VECTORIZED
		path[i] = i;
	    std::random_shuffle(path.begin(), path.end());
	    population[k] = path;
    }
}

void Population::generate_population_thread(int nw){
    std::vector<std::thread> threads;
    int chunk_size = pop_size/nw;

    auto myJob = [this, chunk_size](int k){
	for(int i=k*chunk_size; i<(k+1)*chunk_size; i++){
	    std::vector<int> path = std::vector<int>(n_nodes);
	    for(int j=0; j<n_nodes; j++) // VECTORIZED
		path[j] = j;
	    std::random_shuffle(path.begin(), path.end());
	    population[i] = path;
	}
    };

    // start threads
    for (int i=0; i<nw; i++)
        threads.push_back(std::thread(myJob, i));
    for (int i=0; i<nw; i++)
        threads[i].join();
}

void Population::calculate_affinities(City city){
    double sum = 0;
    for(int k=0; k<pop_size; k++){ // calculate score for every member of population
	double score = city.path_length(population[k]);
	if(score<min_length){
	    min_length = score; // index
	    best_one = population[k]; // path
	}
	affinities[k] = 1/(score+1); // invert score (shortest path are better), +1 to avoid crash
	sum += affinities[k];
    }
    for(int i=0; i<pop_size; i++) // VECTORIZED
	affinities[i] = affinities[i]/sum;
}
// ********************************* crossover and mutation /**********************************/

std::vector<int> Population::crossover(int a, int b, double resistence){
    if(a==b)
	return mutation(population[a], resistence); // same path

    int i = rand()%n_nodes;
    int j = rand()%n_nodes;

    if(i==j)
        return mutation(population[a], resistence); // |list to change| = 0

    if(i>j){  // i must be < j
	int aux = i;
        i = j;
	j = aux;
    }

    std::vector<int> dad = population[a];
    std::vector<int> mom = population[b];

    std::set<int> removed; // elements of dad in [i, j] inclusive are placed in set
    for(int k=0; k<j-i+1; k++)
	removed.insert(dad[i+k]); // from dad[i+0] to dad[i+j-i] = dad[j] inclusive

    for(int k=0; k<j-i+1; k++){ // elements of dad in [i, j] inclusive must be replaced
	int h=0;
	for(; h<n_nodes; h++) // if mom[h] is not in removed, i don't need to see again mom[h]
	    if(removed.find(mom[h])!=removed.end()){ // if h-th elements of mom is in removed
                dad[i+k] = mom[h];
		removed.erase(removed.find(mom[h]));
		break;
    	    }
    }

    return mutation(dad, resistence);
}

std::vector<int> Population::mutation(std::vector<int> a, double resistence){
    int i = rand()%n_nodes;
    int j = rand()%n_nodes;

    double r = ((double) rand() / (RAND_MAX));
    if(r<resistence || i==j)
        return a;

    int aux = a[i];
    a[i] = a[j];
    a[j] = aux;
    return a;
}
