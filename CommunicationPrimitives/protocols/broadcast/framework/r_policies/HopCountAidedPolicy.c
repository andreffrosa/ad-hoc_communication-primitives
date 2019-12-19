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

#include <assert.h>

static bool _HopCountAidedPolicy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {

	int hops = -1;

	for(double_list_item* it = msg_item->copies->head; it; it = it->next) {
		message_copy* msg = ((message_copy*)it->data);

		unsigned char current_hops = 0;
		if(!query_context_header(r_context, msg->context_header, msg->header.context_length, "hops", &current_hops, 0))
			assert(false);

		if(hops == -1)
			hops = current_hops;
		else {
			if( current_hops > hops )
				return false;
		}
	}

	return true;
}

RetransmissionPolicy HopCountAidedPolicy() {
	RetransmissionPolicy r_policy;

	r_policy.args = NULL;
	r_policy.state = NULL;

	r_policy.r_policy = &_HopCountAidedPolicy;

	return r_policy;
}

