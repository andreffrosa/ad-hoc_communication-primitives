
#include "myUtils.h"

#include <assert.h>
#include <stdlib.h>

#define SEC_TO_NANO (1000000000L)

const struct timespec zero_timespec = {0, 0};

double dMin(double a, double b) {
	return a < b ? a : b;
}

double dMax(double a, double b) {
	return a > b ? a : b;
}

long lMin(long a, long b) {
	return a < b ? a : b;
}

long lMax(long a, long b) {
	return a > b ? a : b;
}

int iMin(int a, int b) {
	return a < b ? a : b;
}

int iMax(int a, int b) {
	return a > b ? a : b;
}

int compare_int(int a, int b) {
	if(a == b)
		return 0;
	else if(a < b)
		return -1;
	else
		return 1;
}

/* Function to sort an array using insertion sort
 * Adapted from: https://www.geeksforgeeks.org/insertion-sort/
 * */
void insertionSort(void* v, unsigned int element_size, unsigned int n_elements, int (*cmp)(void*,void*)) {
	int i, j;
	unsigned char key[element_size];
	for (i = 1; i < n_elements; i++) {
		memcpy(key, v + i*element_size, element_size);
		j = i - 1;

		/* Move elements of v[0..i-1], that are
		greater than key, to one position ahead
		of their current position */
		while (j >= 0 && cmp(v + j*element_size, key) > 0) {
			memcpy(v + (j+1)*element_size, v + j*element_size, element_size);
			j = j - 1;
		}

		memcpy(v + (j+1)*element_size, key, element_size);
	}
}

char* buildPath(char* file, char* folder, char* file_name) {
	bzero(file, PATH_MAX);

	strcpy(file, folder);
	if(file[strlen(file)] != '/')
		strcat(file, "/");
	strcat(file, file_name);

	return file;
}

void SetTimer(struct timespec* t, uuid_t id, unsigned short protoID, short int type) {
	YggTimer timer;
	YggTimer_init_with_uuid(&timer, id, protoID, protoID);
	YggTimer_setType(&timer, type);
	YggTimer_set(&timer, t->tv_sec, t->tv_nsec, 0, 0);
	setupTimer(&timer);
}

void SetPeriodicTimer(struct timespec* t, uuid_t id, unsigned short protoID, short int type) {
	YggTimer timer;
	YggTimer_init_with_uuid(&timer, id, protoID, protoID);
	YggTimer_setType(&timer, type);
	YggTimer_set(&timer, t->tv_sec, t->tv_nsec, t->tv_sec, t->tv_nsec);
	setupTimer(&timer);
}

void CancelTimer(uuid_t id, unsigned short protoID) {
	YggTimer timer;
	YggTimer_init_with_uuid(&timer, id, protoID, protoID);
	cancelTimer(&timer);
}

char* timespec_to_string(struct timespec* t, char* str, int len, int precision) {

	bzero(str, len);

	long ns = t->tv_nsec;
	long us = t->tv_nsec / (1000);
	long ms = t->tv_nsec / (1000*1000);
	long sec = t->tv_sec;
	long min = t->tv_sec / 60;
	long h = min / 60;
	long days = h / 24;
	long weeks = days / 7;
	long months = weeks / 30;
	long years = months / 12;

	long tmp[10];
	tmp[0] = years;
	tmp[1] = months - 12*years;
	tmp[2] = weeks - 4*months;
	tmp[3] = days - 7*weeks;
	tmp[4] = h - 24*days;
	tmp[5] = min - 60*h;
	tmp[6] = sec - 60*min;
	tmp[7] = ms;
	tmp[8] = us - 1000*ms;
	tmp[9] = ns - 1000*ms;

	int start = 0;
	for(int i = 0; i < 10; i++) {
		start = i;
		if( tmp[i] > 0 ){
			break;
		}
	}

	int cursor = 0;
	if( start >= 7)
		sprintf(str, "%.3ld", tmp[start]);
	else
		sprintf(str, "%ld", tmp[start]);
	for(int i = start + 1; i < 10 && i < start + precision; i++) {
		cursor = strlen(str);
		assert(cursor < len);
		if( i >= 7)
			sprintf(str + cursor, ":%.3ld", tmp[i]);
		else
			sprintf(str + cursor, ":%ld", tmp[i]);
	}
	char* units = "";
	switch(start){ // Start ou finish?
	case 0: units = "y"; break;
	case 1: units = "mon"; break;
	case 2: units = "w"; break;
	case 3: units = "d"; break;
	case 4: units = "h"; break;
	case 5: units = "min"; break;
	case 6: units = "s"; break;
	case 7: units = "ms"; break;
	case 8: units = "us"; break;
	case 9: units = "ns"; break;
	}

	cursor = strlen(str);
	assert(cursor < len);
	sprintf(str + cursor, " %s", units);

	return str;
}

/* Subtract the `struct timespec' values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0.  */
int subtract_timespec(struct timespec *result, struct timespec *x, struct timespec *y) {

	struct timespec tmp;

	if ((x->tv_sec < y->tv_sec) ||
			((x->tv_sec == y->tv_sec) &&
					(x->tv_nsec <= y->tv_nsec))) {		/* TIME1 <= TIME2? */
		tmp.tv_sec = tmp.tv_nsec = 0 ;
	} else {						/* TIME1 > TIME2 */
		tmp.tv_sec = x->tv_sec - y->tv_sec ;
		if (x->tv_nsec < y->tv_nsec) {
			tmp.tv_nsec = x->tv_nsec + SEC_TO_NANO - y->tv_nsec ;
			tmp.tv_sec-- ;				/* Borrow a second. */
		} else {
			tmp.tv_nsec = x->tv_nsec - y->tv_nsec ;
		}
	}

	memcpy(result, &tmp, sizeof(struct timespec));

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec && (x->tv_sec == y->tv_sec && x->tv_nsec < y->tv_nsec);
}

void add_timespec(struct timespec *result, struct timespec *x, struct timespec *y) {
	long overflow = 0;
	long long nseconds = x->tv_nsec + y->tv_nsec;
	if( nseconds >= SEC_TO_NANO ){
		overflow = (nseconds) / SEC_TO_NANO;
		nseconds = nseconds % SEC_TO_NANO;
	}

	long seconds = x->tv_sec + y->tv_sec + overflow;

	result->tv_nsec = (long)nseconds;
	result->tv_sec 	= seconds;

	assert(result->tv_sec >= 0 && result->tv_nsec >= 0 );
}

void milli_to_timespec(struct timespec *result, unsigned long milli) {

	unsigned long overflow = 0;
	unsigned long n_sec = (milli % 1000) * 1000*1000;
	unsigned long sec = milli / 1000;

	if (n_sec >= SEC_TO_NANO) {
		overflow = (n_sec) / SEC_TO_NANO;
		n_sec = n_sec % SEC_TO_NANO;
	}

	result->tv_nsec = n_sec;
	result->tv_sec = sec + overflow;
}

unsigned long timespec_to_milli(struct timespec* time){
	unsigned long result = 0;

	result += time->tv_sec*1000;
	result += time->tv_nsec/(1000*1000);

	if( time->tv_nsec%(1000*1000) > 500*1000 )
		result += 1;

	return result;
}

unsigned long random_mili_to_timespec(struct timespec* t, unsigned long max_jitter) {
	unsigned long jitter = ((unsigned long)max_jitter*getRandomProb());

	if(t!= NULL)
		milli_to_timespec(t, jitter);

	return jitter;
}

// returns 1 if a is larger ( newer ) than b
/*int compare_timespec(struct timespec* a, struct timespec* b) {

	if (a->tv_sec == b->tv_sec){
		//return a->tv_nsec > b->tv_nsec;
		if( a->tv_nsec > b->tv_nsec )
			return 1;
		else if (a->tv_nsec == b->tv_nsec)
			return 0;
		else
			return -1;
	}
	else{
		//return a->tv_sec > b->tv_sec;
		if( a->tv_sec > b->tv_sec )
			return 1;
		else
			return -1;
	}

}*/

// Both min and max are included in the possible values
/*int getRandomInt(int min, int max) {
	//return (rand() % (max - min + 1)) + min;
	return min + getRandomProb()*(max - min + 1);
}

double getRandomProb() {
	//	return getRandomInt(0, 100) / 100.0;
	double u = ((double)rand()) / RAND_MAX;
	//assert( u >= 0.0 && u <= 1.0 );
	return u;
}*/


void pushToArgs(void** args, int* len, int* argc, void* var, int size){
	int i = *argc;
	args[i] = var;
	len[i] = size;
	(*args)++;
}

void serializeArgs(YggMessage* msg, void** args, int* len, int argc) {

	int size = 0;
	for (int i = 0; i < argc; i++)
		size += len[i];

	void* buffer = malloc(size);
	void* ptr = buffer;

	for (int i = 0; i < argc; i++) {
		memcpy(ptr, args[i], len[i]);
		ptr += len[i];
	}

	YggMessage_addPayload(msg, (char*) buffer, size);

	free(buffer);
}

void deserializeArgs(YggMessage* msg, void** args, int* len, int argc) {

	int size = 0;
	for (int i = 0; i < argc; i++)
		size += len[i];

	void* buffer = malloc(size);

	void* ptr = NULL;
	YggMessage_readPayload(msg,ptr,buffer, size);

	ptr = buffer;
	for (int i = 0; i < argc; i++) {
		memcpy(args[i], ptr, len[i]);
		ptr += len[i];
	}

	free(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////

//https://stackoverflow.com/questions/12762040/get-the-maximum-value-of-a-variable-in-c

long randomLong() {
	int n = ((sizeof(long)/2)*8) - 1;
	long r = rand();
	r = (r << n) | rand();

	r = labs(r);

	return r;
}

int timespec_is_zero(struct timespec* t) {
	return !(t->tv_sec | t->tv_nsec);
}

void clear_timespec(struct timespec* t){
	bzero(t, sizeof(struct timespec));
}

void multiply_timespec(struct timespec* t, struct timespec* time, double alfa) {

	if( alfa == 0 ) {
		t->tv_nsec = 0;
		t->tv_sec = 0;
	} else if( alfa == 1.0 ) {
		t->tv_nsec = time->tv_nsec;
		t->tv_sec = time->tv_sec;
	} else {
		struct timespec temp;

		temp.tv_sec = time->tv_sec * alfa;
		temp.tv_nsec = ((time->tv_sec * alfa) - ((long)(time->tv_sec * alfa)))*1000*1000*1000;

		int beta = alfa / 1;
		for(int i = 0; i < beta; i++) {
			temp.tv_nsec += time->tv_nsec;
			if( temp.tv_nsec >= SEC_TO_NANO ) {
				temp.tv_nsec -= SEC_TO_NANO;
				temp.tv_sec++;
			}
		}

		t->tv_sec = temp.tv_sec;
		t->tv_nsec = temp.tv_nsec;
	}
}


void random_timespec(struct timespec* t, struct timespec* base, struct timespec* interval) {

	assert(interval != NULL);

	//	t->tv_sec = (randomLong() % (interval->tv_sec+1));
	//	if(t->tv_sec == interval->tv_sec)
	//		t->tv_nsec = ((randomLong()) % (interval->tv_nsec >= SEC_TO_NANO ? SEC_TO_NANO : interval->tv_nsec+1));
	//	else
	//		t->tv_nsec = (randomLong() % SEC_TO_NANO);

	double u = getRandomProb();
	multiply_timespec(t, interval, u);

	assert(t->tv_sec >= 0 && t->tv_nsec >= 0 );

	if(base != NULL)
		add_timespec(t, t, base);

	if(timespec_is_zero(t))
		t->tv_nsec = 1;

	assert(t->tv_sec >= 0 && t->tv_nsec >= 0 );
}

void neighboursDelayv1(struct timespec* t, struct timespec* interval, int n_neighbours, double avg_neighbours) {

	// 0.1 is the default value
	double f = (n_neighbours == 0) ? 0.1 : (avg_neighbours / (n_neighbours + avg_neighbours));

	multiply_timespec(t, interval, f);

	struct timespec jitter, max_jitter;
	max_jitter.tv_sec = 0;
	max_jitter.tv_nsec = 5 * 1000 * 1000;
	random_timespec(&jitter, NULL, &max_jitter);

	add_timespec(t, t, &jitter);

	if(timespec_is_zero(t))
		t->tv_nsec = 1;

	/*//
	unsigned long max_delay = 0L;

		double k = getAvgNeighbours();
		int n = getNumNeighbours();
		double f = (n == 0) ? 0.1f : k / (n + k);

	max_delay = min( 1, roundl((long double) f * myArgs->default_timeout));
			//max_delay = roundl((long double) getRandomProb() * max_delay);
			max_delay += roundl(5000 * (long double) getRandomProb());
			break;*/
}

// comparar com a média de vizinhos ou com o max?
void neighboursDelayv2(struct timespec* t, struct timespec* interval, int n_neighbours, double avg_neighbours) {

	// 0.1 is the default value
	double f = (n_neighbours == 0) ? 0.1 : (avg_neighbours / (n_neighbours + avg_neighbours));

	f = round(f*10)*10;

	double jitter = (getRandomInt(0, 1000) / 100.0) - 5;

	double alfa = f + jitter;

	multiply_timespec(t, interval, alfa/100.0);

	if(timespec_is_zero(t))
		t->tv_nsec = 1;
}

double avgNeighFactor(int n_neighbours, double avg_neighbours) {
	if( avg_neighbours == 0.0 ) {
		return 0.0;
	}

	double f = ((double)n_neighbours) / (2.0*avg_neighbours);
	return (f < 0.0) ? 0.0 : ((f > 1.0) ? 1.0 : f);
}

double maxNeighFactor(int n_neighbours, int max_neighbours) {
	if( max_neighbours == 0 ) {
		return 0.0;
	}

	double f = ((double)n_neighbours) / ((double)max_neighbours);
	return (f < 0.0) ? 0.0 : ((f > 1.0) ? 1.0 : f);
}

double minmaxNeighFactor(int n_neighbours, int min_neighbours, int max_neighbours) {
	assert(max_neighbours >= min_neighbours );
	assert(n_neighbours >= 0);

	double interval = (double)(max_neighbours - min_neighbours );

	if( interval == 0 ) {
		return 0.0;
	}

	double f = ((double)(n_neighbours - min_neighbours)) / interval;
	return (f < 0.0) ? 0.0 : ((f > 1.0) ? 1.0 : f);
}

void neighboursDelayv3(struct timespec* t, struct timespec* interval, double neigh_factor) {

	assert(neigh_factor >= 0.0 && neigh_factor <= 1.0);

	double t_factor = (1.0/(0.5*neigh_factor + 0.5))-1.0;

	double slot = round(0.9*t_factor*10);
	double jitter = getRandomProb()*10;

	double alfa = (slot*10 + jitter) / 100.0;

	multiply_timespec(t, interval, alfa);

	if(timespec_is_zero(t))
		t->tv_nsec = 1;

	assert(t->tv_nsec >= 0 && t->tv_sec >= 0);
}

void neighboursDelayv4(struct timespec* t, struct timespec* interval, struct timespec* base, double p, double cdf) {

	assert( p >= 0.0 && p <= 1.0 );
	assert( cdf >= 0.0 && cdf <= 1.0 );
	assert( t!= NULL && interval != NULL );

	double t_factor;

	if( p < 0.01 && cdf < 0.01 ) {
		//p = -1.0;
		t_factor = getRandomProb();
	} else {
		double r = getRandomProb();
		t_factor = cdf - p*r;

	}

	multiply_timespec(t, interval, t_factor);

	if( base != NULL )
		add_timespec(t, t, base);

	if(timespec_is_zero(t))
		t->tv_nsec = 1;

	assert(t->tv_nsec >= 0 && t->tv_sec >= 0);
}

void neighboursDelayv5(struct timespec* t, struct timespec* interval, struct timespec* base, unsigned int n, double p, double cdf) {

	assert( p >= 0.0 && p <= 1.0 );
	assert( cdf >= 0.0 && cdf <= 1.0 );
	assert( t!= NULL && interval != NULL );

	double u = getRandomProb();
	if( n == 0 ) {
		multiply_timespec(t, interval, u);
	} else if( n <= 2 ) { // Only one more neigh
		multiply_timespec(t, interval, u/10.0); // Just to avoid hidden terminal collisions
	} else {
		double t_factor = cdf - p*u;
		struct timespec _interval;
		multiply_timespec(&_interval, interval, dMin(log( n/2.0 ), 1.0));
		multiply_timespec(t, &_interval, t_factor);
	}

	if( base != NULL )
		add_timespec(t, t, base);

	if(timespec_is_zero(t))
		t->tv_nsec = 1;

	assert(t->tv_nsec >= 0 && t->tv_sec >= 0);
}


// From https://www.geeksforgeeks.org/program-to-find-the-next-prime-number/
bool isPrime(unsigned int n) {
	if (n <= 1) return false;
	if (n <= 3) return true;

	// This is checked so that we can skip
	// middle five numbers in below loop
	if (n%2 == 0 || n%3 == 0) return false;

	for (int i=5; i*i<=n; i=i+6)
		if (n%i == 0 || n%(i+2) == 0)
			return false;

	return true;
}

unsigned int nextPrime(unsigned int N) {

	// Base case
	if (N <= 1)
		return 2;

	unsigned int prime = N;
	bool found = false;

	// Loop continuously until isPrime returns
	// true for a number greater than n
	while (!found) {
		prime++;

		if (isPrime(prime))
			found = true;
	}

	return prime;
}



