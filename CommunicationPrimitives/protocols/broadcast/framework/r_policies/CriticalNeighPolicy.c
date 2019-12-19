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

// INCOMPLETE

static bool _CriticalNeighPolicy(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	/*double min_critical_coverage =  *((double*)args);
	unsigned int current_phase = msg_item->current_phase;

	node_type inType = getNodeInType(((message_copy*)msg_item->copies->head->data)->header.sender_id);
	if ( inType == CRITICAL_NODE || inType == UNDEFINED_NODE ) { // Only the critical processes retransmit
		if(current_phase == 1) {
			return true;
		} else {
			double missed_coverage = missedCriticalNeighs(msg_item);
			*//*if(missed > 0)*//*
			if(missed_coverage > min_critical_coverage)
				return true;
		}
	}*/

	return false;
}

RetransmissionPolicy CriticalNeighPolicy(double min_critical_coverage) {

	assert( 0.0 <= min_critical_coverage && min_critical_coverage  <= 1.0 );

	RetransmissionPolicy r_policy;

	r_policy.args = malloc(sizeof(double));
	*((double*)r_policy.args) = min_critical_coverage;
	r_policy.state = NULL;

	r_policy.r_policy = &_CriticalNeighPolicy;

	return r_policy;
}
