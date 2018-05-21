#ifndef _NQ
#define _NQ

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

#include "boardfuncs.c"
#include "populationsearch.c"

// globals
int min_n,max_n,population_size,mutation_chance,repeats,world_size,world_rank;

int main(int argc, char ** argv, char ** envp){
	if(argc < 5){
		printf("Not Enough Arguments!");
		return 1;
	}
	
	MPI_Init(NULL,NULL);
	srand(time(NULL));
	
	min_n = atoi(argv[1]);
	max_n = atoi(argv[2]);
	population_size = atoi(argv[3]);
	mutation_chance = atoi(argv[4]);
	repeats = atoi(argv[5]);
	
	MPI_Comm_size(MPI_COMM_WORLD,&world_size);
	MPI_Comm_rank(MPI_COMM_WORLD,&world_rank);
	
	if(world_rank==0)printf("%i,%i,%i,%i,%i",min_n,max_n,population_size,mutation_chance,repeats);
	
	time_t start,end;
	
	for(int i=min_n;i<=max_n;i++){
		if(world_rank==0){
			start = clock();
			for(int r=0;r<repeats;r++){
				distributed_population_search(i,population_size,mutation_chance,world_size);
			}
			end = clock();
			printf("%i %f\n",i,(double)(end-start)/repeats/CLOCKS_PER_SEC);
		}
		else{
			for(int r=0;r<repeats;r++){
				distributed_population_subprocess(world_rank,i,mutation_chance);
			}
		}		
	}
}

#endif