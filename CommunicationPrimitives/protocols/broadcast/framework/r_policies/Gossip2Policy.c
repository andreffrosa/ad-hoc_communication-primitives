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

typedef struct _Gossip2PolicyArgs {
	double p1;
	unsigned int k;
	double p2;
	unsigned int n;
} Gossip2PolicyArgs;

static bool _Gossip2Policy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	Gossip2PolicyArgs* _args = ((Gossip2PolicyArgs*)args);
	double p1 = _args->p1;
	unsigned int k = _args->k;
	double p2 = _args->p2;
	unsigned int n = _args->n;
	unsigned char hops = 0;
	message_copy* msg = ((message_copy*)msg_item->copies->head->data);
	if(!query_context_header(r_context, msg->context_header, msg->header.context_length, "hops", &hops, 0))
		assert(false);
	int nn = 0;
	if(!query_context_header(r_context, msg->context_header, msg->header.context_length, "neighbors", &nn, 0))
			assert(false);

	double u = getRandomProb();

	return hops < k || (nn < n && u <= p2) || (nn >= n && u <= p1);
}

RetransmissionPolicy Gossip2Policy(double p1, unsigned int k, double p2, unsigned int n) {

	assert( p2 > p1 );

	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(Gossip2PolicyArgs));
	Gossip2PolicyArgs* args = ((Gossip2PolicyArgs*)r_policy.args);
	args->p1 = p1;
	args->k = k;
	args->p2 = p2;
	args->n = n;

	r_policy.state = NULL;

	r_policy.r_policy = &_Gossip2Policy;

	return r_policy;
}

