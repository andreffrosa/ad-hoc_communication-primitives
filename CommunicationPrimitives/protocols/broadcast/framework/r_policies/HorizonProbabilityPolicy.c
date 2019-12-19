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

typedef struct _HorizonProbabilityPolicyArgs {
	double p;
	unsigned int k;
} HorizonProbabilityPolicyArgs;

static bool _HorizonProbabilityPolicy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	HorizonProbabilityPolicyArgs* _args = ((HorizonProbabilityPolicyArgs*)args);
	double p = _args->p;
	unsigned int k = _args->k;
	message_copy* msg = ((message_copy*)msg_item->copies->head->data);
	unsigned char hops = 0;
		if(!query_context_header(r_context, msg->context_header, msg->header.context_length, "hops", &hops, 0))
			assert(false);

	double u = getRandomProb();

	return hops < k || u <= p;
}

RetransmissionPolicy HorizonProbabilityPolicy(double p, unsigned int k) {
	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(HorizonProbabilityPolicyArgs));
	HorizonProbabilityPolicyArgs* args = ((HorizonProbabilityPolicyArgs*)r_policy.args);
	args->p = p;
	args->k = k;

	r_policy.state = NULL;

	r_policy.r_policy = &_HorizonProbabilityPolicy;

	return r_policy;
}

