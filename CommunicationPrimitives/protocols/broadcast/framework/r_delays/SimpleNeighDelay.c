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

// FROM "W. Peng and X. Lu. On the reduction of broadcast redundancy in mobile ad hoc networks. In Proceedings of MOBIHOC, 2000"
static unsigned long getCurrentPhaseDelay(unsigned long t, unsigned int n, unsigned int max_n) {
	double u = getRandomProb();
	double t0 = ((double)(1 + max_n)) / (1 + n);
	return (unsigned long) roundl( t*t0*u );
}

static unsigned long _SimpleNeighDelay(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	unsigned long t = *((unsigned long*) args);

	unsigned int n, max_n;
	if(!query_context(r_context, "neighbors", &n, 0))
		assert(false);
	if(!query_context(r_context, "max_neighbors", &max_n, 0))
		assert(false);

	return getCurrentPhaseDelay(t, n, max_n);
}

RetransmissionDelay SimpleNeighDelay(unsigned long t) {
	RetransmissionDelay r_delay;

	r_delay.args = malloc(sizeof(t));
	*((unsigned long*)r_delay.args) = t;

	r_delay.state = NULL;
	r_delay.r_delay = &_SimpleNeighDelay;

	return r_delay;
}
