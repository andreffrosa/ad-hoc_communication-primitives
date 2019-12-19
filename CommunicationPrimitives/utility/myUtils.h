/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2019
 *********************************************************/

#ifndef MY_UTILS_H_
#define MY_UTILS_H_

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <limits.h>
#include <math.h>

#include "../Yggdrasil/Yggdrasil/core/utils/utils.h"
#include "../Yggdrasil/Yggdrasil/core/proto_data_struct.h"
#include "Yggdrasil_lowlvl.h"

#include "../Yggdrasil/Yggdrasil/core/protos/timer.h"

// Min and Max
long lMin(long a, long b);
long lMax(long a, long b);
double dMin(double a, double b);
double dMax(double a, double b);
int iMin(int a, int b);
int iMax(int a, int b);

bool isPrime(unsigned int n);
unsigned int nextPrime(unsigned int N);
void insertionSort(void* v, unsigned int element_size, unsigned int n_elements, int (*cmp)(void*,void*));
int compare_int(int a, int b);

// Random
//int getRandomInt(int min, int max);
//double getRandomProb();
long randomLong();

// Path
char* buildPath(char* file, char* folder, char* file_name);

// Timespec
extern const struct timespec zero_timespec;

char* timespec_to_string(struct timespec* t, char* str, int len, int precision);
int subtract_timespec(struct timespec *result, struct timespec *x, struct timespec *y);
void add_timespec(struct timespec *result, struct timespec *x, struct timespec *y);
//int compare_timespec(struct timespec* a, struct timespec* b);
void clear_timespec(struct timespec* t);
int timespec_is_zero(struct timespec* t);
void multiply_timespec(struct timespec* t, struct timespec* time, double alfa);
void random_timespec(struct timespec* t, struct timespec* base, struct timespec* interval);
void milli_to_timespec(struct timespec *result, unsigned long milli );
unsigned long timespec_to_milli(struct timespec* time);
unsigned long random_mili_to_timespec(struct timespec* t, unsigned long max_jitter);

// Timer
void SetTimer(struct timespec* t, uuid_t id, unsigned short protoID, short int type);
void SetPeriodicTimer(struct timespec* t, uuid_t id, unsigned short protoID, short int type);
void CancelTimer(uuid_t id, unsigned short protoID);

#endif /* MY_UTILS_H_ */
