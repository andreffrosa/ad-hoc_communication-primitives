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

#ifndef _BCAST_ALGORITHMS_H_
#define _BCAST_ALGORITHMS_H_

#include "modules.h"

#include "r_contexts/r_contexts.h"
#include "r_delays/r_delays.h"
#include "r_policies/r_policies.h"

BroadcastAlgorithm newBcastAlgorithm(RetransmissionContext r_context, RetransmissionDelay r_delay, RetransmissionPolicy r_policy, unsigned int n_phases) ;

BroadcastAlgorithm Flooding(unsigned long t);
BroadcastAlgorithm Gossip1(unsigned long t, double p);
BroadcastAlgorithm Gossip1_hops(unsigned long t, double p, unsigned int k);
BroadcastAlgorithm Gossip2(unsigned long t, double p1, unsigned int k, double p2, unsigned int n);
BroadcastAlgorithm RAPID(unsigned long t, double beta);
BroadcastAlgorithm EnhancedRAPID(unsigned long t1, unsigned long t2, double beta);
BroadcastAlgorithm Gossip3(unsigned long t1, unsigned long t2, double p, unsigned int k, unsigned int m);
BroadcastAlgorithm Counting(unsigned long t, unsigned int c);
BroadcastAlgorithm HopCountAided(unsigned long t);

BroadcastAlgorithm NABA1(unsigned long t, unsigned int c); // CountingNABA
BroadcastAlgorithm NABA2(unsigned long t, unsigned int c1, unsigned int c2); // PbCountingNABA
BroadcastAlgorithm NABA3(unsigned long t, double min_critical_coverage);
BroadcastAlgorithm NABA4(unsigned long t, double min_critical_coverage);
BroadcastAlgorithm NABA3e4(unsigned long t, double min_critical_coverage, unsigned int np); // CriticalNABA
//BroadcastAlgorithm NABA5(unsigned long t, double min_critical_coverage, unsigned int np); // EnhancedCriticalNABA ou Advanced ou semelhante -> Só pode ter 1 fase = Verificar coberta caso contrário retransmitir se for critico

#endif /* _BCAST_ALGORITHMS_H_ */
