#ifndef _BOARDFUNCS
#define _BOARDFUNCS

int get_board_heuristic(int * b, int n);
void print_board_small(int * b, int n);
int population_comparator(const void * p, const void * q, void * n);
int difference_factor(int * x, int * y, int n);
int * population_merge_boards(int * a, int * b, int * c, int n, int mut_chance);

int get_board_heuristic(int * b,int n) {
	int h=0;
	for (int i = 0;i < n;i++) {
		for (int j = i+1; j < n;j++) {
			if (b[i] == b[j]) h++;
			if (abs(b[i] - b[j]) == abs(i-j)) h++;
		}
	}
	return h;
}

void print_board_small(int * b, int n){
	for(int i=0;i<n;i++){
		printf("%i ",b[i]);
	}
	printf("\n");
}

// get the difference between two board states. 0 = none same, 1 = 1 same, ... n-1 = n-1 same
int difference_factor(int * x, int * y, int n){
	int h=0;
	for(int i=0;i<n;i++){
		if(x[i]==y[i]) h++;
	}
	return h;
}

// comparator for sorting board states with qsort_r
int population_comparator(const void *p, const void *q,void * n) {
	int size = *(int*)n;
	int * l = *(int**)p;
	int * r = *(int**)q;
	return (get_board_heuristic(l,size) - get_board_heuristic(r,size));
}

// comparator for sorting board states with qsort_r
#ifdef __linux__
int population_based_search_comparator(const void *p, const void *q,void * n) {
#else
int population_based_search_comparator(void * n,const void *p, const void *q) {
#endif
	int size = *(int*)n;
	int * l = *(int**)p;
	int * r = *(int**)q;
	return ((l[size]+l[size+1]) - (r[size] + r[size+1]));
}

int * population_merge_boards(int * a, int * b, int * c, int n, int mut_chance){
	
	int r;
	
	for(int i=0;i<n;i++){
		
		c[i]=a[i];
		
	}
	
	for(int i=0;i<n/2;i++){
		
		c[i] = b[i];
		
	}
	
	if ((rand() % 100) < mut_chance){
		c[rand()%n]=rand()%n;
	}
	
	return c;
}

#endif