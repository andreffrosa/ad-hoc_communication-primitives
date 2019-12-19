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

static unsigned long _RandomDelay(void* args, void* state, message_item* msg, RetransmissionContext* r_context) {
	unsigned long t = *((unsigned long*) args);

	double u = getRandomProb();
	unsigned long delay = (unsigned long) roundl(u*t);
	return delay;
}

RetransmissionDelay RandomDelay(unsigned long t) {
	RetransmissionDelay r_delay;

	r_delay.args = malloc(sizeof(t));
	*((unsigned long*)r_delay.args) = t;

	r_delay.state = NULL;
	r_delay.r_delay = &_RandomDelay;

	return r_delay;
}
