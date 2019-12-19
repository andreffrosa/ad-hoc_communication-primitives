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

static bool _EnhancedRapidPolicy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	double beta = *((double*)args);
	unsigned int n = 0;
	if(!query_context(r_context, "n_neighbors", &n, 0))
		assert(false);
	unsigned int n_copies = msg_item ->copies->size;
	unsigned int current_phase = msg_item->current_phase;
	unsigned int retransmissions = msg_item->retransmissions;

	double p  = dMin(1.0, (beta / n));
	double u = getRandomProb();

	if(current_phase == 1) {
		return (u <= p) && n_copies == 1;
	} else {
		if(retransmissions == 0) {
			unsigned int copies_on_2nd_phase = 0;

			double_list_item* it = msg_item->copies->head;
			while(it) {
				if(((message_copy*)it->data)->phase == 2) {
					copies_on_2nd_phase++;
				}
				it = it->next;
			}

			return copies_on_2nd_phase == 0;
		} else
			return false;
	}
}

RetransmissionPolicy EnhancedRapidPolicy(double beta) {

	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(double));
	*((double*)r_policy.args) = beta;

	r_policy.state = NULL;
	r_policy.r_policy = &_EnhancedRapidPolicy;

	return r_policy;
}

