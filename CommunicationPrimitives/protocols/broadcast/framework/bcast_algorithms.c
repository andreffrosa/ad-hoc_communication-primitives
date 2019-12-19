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

#include "bcast_algorithms.h"

BroadcastAlgorithm newBcastAlgorithm(RetransmissionContext r_context, RetransmissionDelay r_delay, RetransmissionPolicy r_policy, unsigned int n_phases) {
	BroadcastAlgorithm alg;

	alg.r_context = r_context;
	alg.r_delay = r_delay;
	alg.r_policy = r_policy;
	alg.n_phases = n_phases;

	return alg;
}

BroadcastAlgorithm Flooding(unsigned long t) {
	return newBcastAlgorithm(EmptyContext(), RandomDelay(t), TruePolicy(), 1);
}

BroadcastAlgorithm Gossip1(unsigned long t, double p) {
	return newBcastAlgorithm(EmptyContext(), RandomDelay(t), ProbabilityPolicy(p), 1);
}

BroadcastAlgorithm Gossip1_hops(unsigned long t, double p, unsigned int k) {
	return newBcastAlgorithm(HopsContext(), RandomDelay(t), HorizonProbabilityPolicy(p, k), 1);
}

BroadcastAlgorithm Gossip2(unsigned long t, double p1, unsigned int k, double p2, unsigned int n) {
	topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
	new_topology_discovery_args(d_args, 1, 500, 5000, 60*1000, 3); // TODO: passar estes valores por arg?
	return newBcastAlgorithm(ComposeContext(2, HopsContext(), NeighborsContext(d_args, true)), RandomDelay(t), Gossip2Policy(p1, k, p2, n), 1);
}

BroadcastAlgorithm RAPID(unsigned long t, double beta) {
	topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
	new_topology_discovery_args(d_args, 1, 500, 5000, 60*1000, 3); // TODO: passar estes valores por arg?
	return newBcastAlgorithm(NeighborsContext(d_args, false), RandomDelay(t), RapidPolicy(beta), 1);
}

BroadcastAlgorithm EnhancedRAPID(unsigned long t1, unsigned long t2, double beta) {
	topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
	new_topology_discovery_args(d_args, 1, 500, 5000, 60*1000, 3); // TODO: passar estes valores por arg?
	return newBcastAlgorithm(NeighborsContext(d_args, false), TwoPhaseRandomDelay(t1, t2), EnhancedRapidPolicy(beta), 2);
}

BroadcastAlgorithm Gossip3(unsigned long t1, unsigned long t2, double p, unsigned int k, unsigned int m) {
	return newBcastAlgorithm(HopsContext(), TwoPhaseRandomDelay(t1, t2), Gossip3Policy(p, k, m), 2);
}

BroadcastAlgorithm Counting(unsigned long t, unsigned int c) {
	return newBcastAlgorithm(EmptyContext(), RandomDelay(t), CountPolicy(c), 1);
}

BroadcastAlgorithm HopCountAided(unsigned long t) {
	return newBcastAlgorithm(HopsContext(), RandomDelay(t), HopCountAidedPolicy(), 1);
}

BroadcastAlgorithm NABA1(unsigned long t, unsigned int c) {
	topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
	new_topology_discovery_args(d_args, 1, 500, 5000, 60*1000, 3); // TODO: passar estes valores por arg?
	return newBcastAlgorithm(NeighborsContext(d_args, false), DensityNeighDelay(t), NeighborCountingPolicy(c), 1);
}

BroadcastAlgorithm NABA2(unsigned long t, unsigned int c1, unsigned int c2) {
	topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
	new_topology_discovery_args(d_args, 1, 500, 5000, 60*1000, 3); // TODO: passar estes valores por arg?
	return newBcastAlgorithm(NeighborsContext(d_args, false), DensityNeighDelay(t), PbNeighCountingPolicy(c1, c2), 1);
}

// TODO: Especificar como algs diferentes ou como o mesmo com outro param? Mesma situação para o Rapid
BroadcastAlgorithm NABA3(unsigned long t, double min_critical_coverage) {
	return newBcastAlgorithm(LabelNeighsContext(), DensityNeighDelay(t), CriticalNeighPolicy(min_critical_coverage), 1);
}

BroadcastAlgorithm NABA4(unsigned long t, double min_critical_coverage) {
	return newBcastAlgorithm(LabelNeighsContext(), DensityNeighDelay(t), CriticalNeighPolicy(min_critical_coverage), 2);
}

BroadcastAlgorithm NABA3e4(unsigned long t, double min_critical_coverage, unsigned int np) {
	return newBcastAlgorithm(LabelNeighsContext(), DensityNeighDelay(t), CriticalNeighPolicy(min_critical_coverage), np);
}


