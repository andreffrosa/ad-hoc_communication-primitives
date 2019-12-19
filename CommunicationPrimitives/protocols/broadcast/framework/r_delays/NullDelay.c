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

#include "r_delays.h"

#include "utility/myUtils.h"

static unsigned long _NullDelay(void* args, void* state, message_item* msg, RetransmissionContext* r_context) {
	return 0L;
}

RetransmissionDelay NullDelay() {
	RetransmissionDelay r_delay;

	r_delay.args = NULL;
	r_delay.state = NULL;
	r_delay.r_delay = &_NullDelay;

	return r_delay;
}
