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

#include "r_policies.h"

#include "utility/myUtils.h"

#include <assert.h>

static bool _PbNeighCountingPolicy(void* args, void* state, message_item* msg, RetransmissionContext* r_context) {
	unsigned int c1 = ((unsigned int*)args)[0], c2 = ((unsigned int*)args)[1];
	unsigned int n_copies = msg->copies->size;
	unsigned int n = 0;
		if(!query_context(r_context, "n_neighbors", &n, 0))
			assert(false);

	if(n_copies < n) {
		if(n_copies <= c1) {
			return true;
		} else if(n_copies < c2) {
			double p = (c2 - n_copies) / ((double) (c2 - c1)); // Probability of retransmitting
			return (getRandomProb() <= p);
		}
	}

	return false;
}

RetransmissionPolicy PbNeighCountingPolicy(unsigned int c1, unsigned int c2) {
	RetransmissionPolicy r_policy;

	r_policy.args = malloc(2*sizeof(unsigned int));
	((unsigned int*)r_policy.args)[0] = c1;
	((unsigned int*)r_policy.args)[1] = c2;
	r_policy.state = NULL;

	r_policy.r_policy = &_PbNeighCountingPolicy;

	return r_policy;
}
