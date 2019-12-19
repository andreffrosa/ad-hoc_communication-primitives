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

static bool _RapidPolicy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	double beta = *((double*)args);
	unsigned int n = 0;
	if(!query_context(r_context, "n_neighbors", &n, 0))
		assert(false);

	double p  = dMin(1.0, (beta / n));
	double u = getRandomProb();

	return u <= p;
}

RetransmissionPolicy RapidPolicy(double beta) {

	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(double));
	*((double*)r_policy.args) = beta;

	r_policy.state = NULL;
	r_policy.r_policy = &_RapidPolicy;

	return r_policy;
}

