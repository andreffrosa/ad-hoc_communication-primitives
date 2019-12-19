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

static bool _CountPolicy(void* args, void* state, message_item* msg, RetransmissionContext* r_context) {
	unsigned int c = *((unsigned int*)args);
	unsigned int n_copies = msg->copies->size;

	return n_copies < c;
}

RetransmissionPolicy CountPolicy(unsigned int c) {
	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(c));
	*((unsigned int*)r_policy.args) = c;
	r_policy.state = NULL;

	r_policy.r_policy = &_CountPolicy;

	return r_policy;
}
