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

#include "r_delays.h"

#include <math.h>
#include <assert.h>

#include "utility/myUtils.h"

static unsigned long getCurrentPhaseDelay(unsigned long t, unsigned int n, double p, double cdf) {
	double u = getRandomProb();
	if( n == 0 ) {
		return (unsigned long) roundl(u*t);
	} else if( n <= 2 ) { // Only one more neigh
		return (unsigned long) roundl((u/10.0)*t); // Just to avoid hidden terminal collisions
	} else {
		double t_factor = cdf - p*u;
		double _interval = ( dMin(log( n/2.0 ), 1.0) * t );
		return (unsigned long) roundl( t_factor * _interval );
	}
}

static unsigned long _DensityNeighDelay(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	unsigned long t = *((unsigned long*) args);

	unsigned long remaining = t - msg_item->timer_duration;

	double aux[3];
	if(!query_context(r_context, "neighbors_distribution", aux, 0))
		assert(false);

	return remaining + getCurrentPhaseDelay(t, (int)aux[0], aux[1], aux[2]);
}

RetransmissionDelay DensityNeighDelay(unsigned long t) {
	RetransmissionDelay r_delay;

	r_delay.args = malloc(sizeof(t));
	*((unsigned long*)r_delay.args) = t;

	r_delay.state = NULL;
	r_delay.r_delay = &_DensityNeighDelay;

	return r_delay;
}
