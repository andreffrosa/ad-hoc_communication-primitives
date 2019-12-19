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

static unsigned long _TwoPhaseRandomDelay(void* args, void* state, message_item* msg_item, RetransmissionContext* r_context) {
	unsigned long t1 = ((unsigned long*)args)[0];
	unsigned long t2 = ((unsigned long*)args)[1];
	unsigned int current_phase = msg_item->current_phase;

	double u = getRandomProb();

	unsigned long delay;
	if(current_phase == 1)
		delay = (unsigned long) roundl(u*t1);
	else
		delay = (unsigned long) roundl(u*t2);

	return delay;
}

RetransmissionDelay TwoPhaseRandomDelay(unsigned long t1, unsigned long t2) {
	RetransmissionDelay r_delay;

	r_delay.args = malloc(2*sizeof(unsigned long));
	unsigned long* t = ((unsigned long*)r_delay.args);
	t[0] = t1;
	t[1] = t2;

	r_delay.state = NULL;
	r_delay.r_delay = &_TwoPhaseRandomDelay;

	return r_delay;
}
