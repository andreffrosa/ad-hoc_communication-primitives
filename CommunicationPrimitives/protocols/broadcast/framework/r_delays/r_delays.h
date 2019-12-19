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

#ifndef _R_DELAYS_H_
#define _R_DELAYS_H_

#include "../modules.h"

RetransmissionDelay NullDelay();
RetransmissionDelay RandomDelay(unsigned long max);
RetransmissionDelay TwoPhaseRandomDelay(unsigned long t1, unsigned long t2);
RetransmissionDelay SimpleNeighDelay(unsigned long max);
RetransmissionDelay DensityNeighDelay(unsigned long max);

#endif /* _R_DELAYS_H_ */
