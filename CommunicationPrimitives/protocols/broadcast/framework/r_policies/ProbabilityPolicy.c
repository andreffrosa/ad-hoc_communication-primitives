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

static bool _ProbabilityPolicy(void* args, void* state, message_item* msg, RetransmissionContext* r_context) {
	double p = *((double*)args);
	double u = getRandomProb();

	return u <= p;
}

RetransmissionPolicy ProbabilityPolicy(double p) {
	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(p));
	*((double*)r_policy.args) = p;
	r_policy.state = NULL;

	r_policy.r_policy = &_ProbabilityPolicy;

	return r_policy;
}

