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

#ifndef PROTOCOLS_DISCOVERY_TOPOLOGY_DISCOVERY_H_
#define PROTOCOLS_DISCOVERY_TOPOLOGY_DISCOVERY_H_

#include "Yggdrasil_lowlvl.h"

#define TOPOLOGY_DISCOVERY_PROTOCOL 159

typedef enum {
	NEIGHBOR_FOUND,
	NEIGHBOR_UPDATE,
	NEIGHBOR_LOST,
	NEIGHBORHOOD_UPDATE,
	DISCOVERY_EVENT_COUNT
} topology_discovery_events;

typedef struct _topology_discovery_stats {
	unsigned int new_neighbours;
	unsigned int lost_neighbours;
	unsigned int announces;
	unsigned int piggybacked_heartbeats;
	unsigned int missed_requests;
	unsigned int misses;
} topology_discovery_stats;

typedef enum {
	STATS_REQ = 0
} topology_discovery_requests;

typedef struct _topology_discovery_args {
	unsigned char horizon; // Max number of hops to discover neighbors
	unsigned long max_jitter; // Interval in which the random delay is calculated (ms)
	unsigned long heartbeat_period; // Maximum time between announcements (ms)
	unsigned long t_gc; // Maximum time between garbage collector executions (ms)
	unsigned int misses; // Maximum heartbeats missed to consider neighbor as a failed node
} topology_discovery_args;

void new_topology_discovery_args(topology_discovery_args* d_args, unsigned int horizon, unsigned long max_jitter, unsigned long heartbeat_period, unsigned long t_gc, unsigned int max_heartbeats_missed_to_consider_failed);
proto_def* topology_discovery_init(void* arg);

void processAnnounce(void* state, void* announce, int size, int horizon, void (*process_node)(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id, unsigned char n_neighs), void (*process_edge)(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id1, unsigned char* id2));
void printAnnounce(void* announce, int size, int horizon, char** str);

#endif /* PROTOCOLS_DISCOVERY_TOPOLOGY_DISCOVERY_H_ */
