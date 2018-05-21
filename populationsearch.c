#ifndef _POPULATIONSEARCH
#define _POPULATIONSEARCH

// Extra Data At the end of each board

// Extra Index 1 (board_size+1): Board Heuristic
// Extra Index 2 (board_size+2): Board Difference Factor
// Extra Index 3 (board_size+3): Parent 1
// Extra Index 4 (board_size+4): Parent 2
// Tracking these allows for the subprocess to do the following:
// 1. Recieve two boards and generate crossover
// 2. Calculate the heuristic of the board it generates
// 3. Calculate the difference factors w.r.t Parent 1 and 2 (Those can be ignored when it is tested later on)


#define EXTDATA 4

int distributed_population_search(int board_size,int population_size,int mutation_chance,int world_size);
int distributed_population_subprocess(int world_rank,int board_size,int mutation_chance);

int distributed_population_search(int board_size,int population_size,int mutation_chance,int world_size){
	
	int population_size_squared = population_size*population_size, true_board_size = board_size + EXTDATA;
	
	int ** boards = (int**)malloc(population_size_squared*sizeof(int*));
	
	for(int i=0;i<population_size_squared;i++){
		boards[i]=(int*)malloc(true_board_size*sizeof(int));
		if (i < population_size) {
			for(int j=0;j<board_size;j++){
				boards[i][j]=rand()%board_size;
			}
		}
	}
	
	int generations = 0;
	
	int available_threads = world_size;
	
	int solution_state = 0; 
	
	int sent_boards = 0, recieved_boards=0;
	
	int interacting_subprocess=0;
	
	MPI_Request request;
	MPI_Status status;
	int request_complete;
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	while(!solution_state){
		
		generations++;
		
		int current_board_i=0,current_board_j=1;
		
		recieved_boards = population_size;
		sent_boards = population_size;
		
		while(recieved_boards < population_size_squared-1){
			
			// if there are outstanding boards
			if(sent_boards > recieved_boards){
				
				// nonblocking recv to see if any boards are ready
				MPI_Irecv(boards[recieved_boards],true_board_size,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&request);
				MPI_Test(&request,&request_complete,&status);
				
				// if so, recieve board and insert it into the most recent open slot,
				if(request_complete!=0){
					// and increment the recieved_boards variable.
					recieved_boards++;
				}
			}
			
			if(sent_boards < population_size_squared-1){
				
				// nonblocking recieve to see if any subprocesses are available
				MPI_Irecv(&interacting_subprocess,1,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&request);
				MPI_Test(&request,&request_complete,&status);
				
				// if so, send board data and increment the sent_boards variable
				if(request_complete!=0){
					MPI_Ssend(boards[current_board_i],true_board_size,MPI_INT,interacting_subprocess,0,MPI_COMM_WORLD);
					MPI_Ssend(boards[current_board_j],true_board_size,MPI_INT,interacting_subprocess,0,MPI_COMM_WORLD);
					current_board_j++;
					if(current_board_j == current_board_i) current_board_j++;
					if(current_board_j == board_size){
						current_board_i++;
						current_board_j=0;
					}
					sent_boards++;
				}
			}
		}
		
		for(int i=0;i<population_size_squared;i++){
			
			if (boards[i][board_size]==0){
				solution_state=1;
				break;
			}
			
			for(int j=0;j<population_size_squared;j++){
				if (j == boards[i][board_size+2] || j==boards[i][board_size+3]){
					continue;
				}
				boards[i][board_size+1] += difference_factor(boards[i],boards[j],board_size);
			}
			boards[i][board_size+1] /= (population_size_squared);
		}
		#ifdef __linux__
		qsort_r(boards,population_size_squared,sizeof(int**),population_based_search_comparator,&board_size);
		#else
		qsort_r(boards,population_size_squared,sizeof(int**),&board_size,population_based_search_comparator);
		#endif
	}
	
	int exit_rcv=-1;
	int exit_msg=-1;
	
	for(int i=0;i<world_size-1;i++){
		MPI_Recv(&exit_rcv,1,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		MPI_Send(&exit_msg,1,MPI_INT,exit_rcv,1,MPI_COMM_WORLD);
	}
	
	for(int i=0;i<population_size_squared;i++){
		free(boards[i]);
	}
	
	return generations;
	
}

int distributed_population_subprocess(int world_rank,int board_size,int mutation_chance){
	
	int process_id=world_rank,mail=0,is_running=1;
	
	int true_board_size = board_size + EXTDATA;
	
	int * board_a, * board_b, * board_c;

	board_a = (int*)malloc(sizeof(int)*board_size);
	board_b = (int*)malloc(sizeof(int)*board_size);
	board_c = (int*)malloc(sizeof(int)*board_size);
	
	MPI_Barrier(MPI_COMM_WORLD);

	while(is_running){
		
		// Wait for master to recieve ready signal
		MPI_Ssend(&process_id,1,MPI_INT,0,1,MPI_COMM_WORLD);
		
		// Recieve Response From Master
		MPI_Recv(&mail,1,MPI_INT,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		
		// If mail == -1, a solution has been reached and this program can terminate
		if(mail==-1)break;
		
		// Else, recieve two boards from the master
		MPI_Recv(board_a,true_board_size,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		MPI_Recv(board_b,true_board_size,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		
		// Modify Board
		population_merge_boards(board_a,board_b,board_c,board_size,mutation_chance);
		
		// Get Current Board Heuristic
		board_c[board_size]=get_board_heuristic(board_c,board_size);
		board_c[board_size+1]=difference_factor(board_a,board_c,board_size) + 
							  difference_factor(board_b,board_c,board_size);
		
		// Send Back Modified Board Data
		MPI_Ssend(board_c,true_board_size,MPI_INT,0,0,MPI_COMM_WORLD);
		
	}
	
	free(board_a);
	free(board_b);
	free(board_c);
	
	return 0;
	
}

#endif