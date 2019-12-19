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

#ifndef _R_CONTEXTS_H_
#define _R_CONTEXTS_H_

#include "../modules.h"

#include "protocols/discovery/topology_discovery.h"

RetransmissionContext EmptyContext();

RetransmissionContext HopsContext();

RetransmissionContext ParentsContext();

RetransmissionContext NeighborsContext(topology_discovery_args* d_args, bool append_neighbors);

RetransmissionContext LabelNeighsContext();
typedef enum _LabelNeighs_NodeLabel {
	UNDEFINED_NODE, CRITICAL_NODE, COVERED_NODE, REDUNDANT_NODE
} LabelNeighs_NodeLabel;

RetransmissionContext ComposeContext(int amount, ...);

#endif /* _R_CONTEXTS_H_ */
