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

typedef struct _Gossip3PolicyArgs {
	double p;
	unsigned int k;
	unsigned int m;
} Gossip3PolicyArgs;

static bool _Gossip3Policy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	Gossip3PolicyArgs* _args = ((Gossip3PolicyArgs*)args);
	double p = _args->p;
	unsigned int k = _args->k;
	unsigned int m = _args->m;
	message_copy* msg = ((message_copy*)msg_item->copies->head->data);
	unsigned char hops = 0;
			if(!query_context_header(r_context, msg->context_header, msg->header.context_length, "hops", &hops, 0))
				assert(false);
	unsigned int n_copies = msg_item->copies->size;
	unsigned int current_phase = msg_item->current_phase;
	unsigned int retransmissions = msg_item->retransmissions;

	if(current_phase == 1) {
		double u = getRandomProb();
		return hops < k || u <= p;
	} else {
		if(retransmissions == 0) {
			return n_copies < m;
		} else {
			return false;
		}
	}
}

RetransmissionPolicy Gossip3Policy(double p, unsigned int k, unsigned int m) {
	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(Gossip3PolicyArgs));
	Gossip3PolicyArgs* args = ((Gossip3PolicyArgs*)r_policy.args);
	args->p = p;
	args->k = k;
	args->m = m;

	r_policy.state = NULL;

	r_policy.r_policy = &_Gossip3Policy;

	return r_policy;
}

