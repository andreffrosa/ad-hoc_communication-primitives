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

#include "r_contexts.h"

#include "data_structures/hash_table.h"
#include "utility/myUtils.h"
#include "../Yggdrasil/Yggdrasil/core/utils/hashfunctions.h"
#include "data_structures/graph.h"

#include <assert.h>

typedef struct _LabelNeighsContextState {
	RetransmissionContext neighbors_context;
	hash_table* direct_neighbors;
} LabelNeighsContextState;

/*typedef struct _LabelNeighsContextArgs {
	topology_discovery_args* d_args;
	bool append_neighbors;
} LabelNeighsContextArgs;*/

typedef struct _dic_entry {
	LabelNeighs_NodeLabel inLabel;
	LabelNeighs_NodeLabel outLabel;
} dic_entry;

// TODO: Compute the labels

static double missedCriticalNeighs(message_item* msg_item, graph* neighborhood, unsigned char* myID);

static bool equalID(uuid_t a, uuid_t b) {
	return uuid_compare(a, b) == 0;
}

static unsigned long uuid_hash(uuid_t id) {
	return RSHash(id, sizeof(uuid_t));
}

static void* LabelNeighsContextInit(void* context_args, proto_def* protocol_definition) {
	LabelNeighsContextState* state = malloc(sizeof(LabelNeighsContextState));

	topology_discovery_args* d_args = malloc(sizeof(topology_discovery_args));
	new_topology_discovery_args(d_args, 2, 500, 5000, 60*1000, 3); // TODO: pass by arg?
	state->neighbors_context = NeighborsContext(d_args, false);

	state->neighbors_context.init(state->neighbors_context.context_args, protocol_definition);

	state->direct_neighbors = hash_table_init((hashing_function) &uuid_hash, (comparator_function) &equalID);

	return state;
}

static unsigned int LabelNeighsContextHeader(void* context_args, void* context_state, message_item* msg, void** context_header, RetransmissionContext* r_context) {
	LabelNeighsContextState* state = (LabelNeighsContextState*)state;

	return state->neighbors_context.create_header(state->neighbors_context.context_args, state->neighbors_context.context_state, msg, context_header, r_context);
}

static void LabelNeighsContextEvent(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context) {

	LabelNeighsContextState* state = (LabelNeighsContextState*)state;
	state->neighbors_context.process_event(state->neighbors_context.context_args, state->neighbors_context.context_state, elem, r_context);

	// TODO: Update labels

	if(elem->type == YGG_EVENT) {
		if(elem->data.event.notification_id == NEIGHBOR_FOUND) {
			// TODO:
		} else if(elem->data.event.notification_id == NEIGHBOR_UPDATE) {
			// TODO:
		} else if(elem->data.event.notification_id == NEIGHBOR_LOST) {
			// TODO:
		}
	}
}

static bool LabelNeighsContextQuery(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {

	LabelNeighsContextState* state = (LabelNeighsContextState*)state;

	if(strcmp(query, "node_inLabel") == 0) {
		assert(argc >= 1);
		unsigned char* id = va_arg(*argv, unsigned char*);
		hash_table_item* it = hash_table_find_item(state->direct_neighbors, id);
		if(it!=NULL){
			*((LabelNeighs_NodeLabel*)result) = ((dic_entry*)it->value)->inLabel;
			return true;
		} else
			return false;
	} else if(strcmp(query, "node_outLabel") == 0) {
		assert(argc >= 1);
		unsigned char* id = va_arg(*argv, unsigned char*);
		hash_table_item* it = hash_table_find_item(state->direct_neighbors, id);
		if(it!=NULL){
			*((LabelNeighs_NodeLabel*)result) = ((dic_entry*)it->value)->outLabel;
			return true;
		} else
			return false;
	} else if(strcmp(query, "missed_critical_neighs") == 0) {
		assert(argc >= 1);
		message_item* msg_item = va_arg(*argv, message_item*);

		graph* neighborhood;
		bool aux = query_context(&state->neighbors_context, "graph", &neighborhood, 0);
		assert(aux == true);

		uuid_t myID;
		aux = query_context(&state->neighbors_context, "my_id", &myID, 0);
		assert(aux == true);

		*((double*)result) = missedCriticalNeighs(msg_item, neighborhood, myID);
		return true;
	} else {
		return state->neighbors_context.query_handler(state->neighbors_context.context_args, state->neighbors_context.context_state, query, result, argc, argv, r_context);
	}
}

static bool LabelNeighsContextQueryHeader(void* context_args, void* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	//TODO
	return false;
}

RetransmissionContext LabelNeighsContext() {
	RetransmissionContext r_context;

	r_context.context_args = NULL;
	r_context.context_state = NULL;

	r_context.init = &LabelNeighsContextInit;
	r_context.create_header = &LabelNeighsContextHeader;
	r_context.process_event = &LabelNeighsContextEvent;
	r_context.query_handler = &LabelNeighsContextQuery;
	r_context.query_header_handler = &LabelNeighsContextQueryHeader;

	return r_context;
}

static double missedCriticalNeighs(message_item* msg_item, graph* neighborhood, unsigned char* myID) {
	unsigned int missed = 0;
	unsigned int critical_neighs = 0;

	graph_item* node = graph_find_item(neighborhood, myID);
	assert(node != NULL);
	for(int i = 0; i < node->degree; i++) {
		unsigned char* neigh_id = neighborhood->nodes[node->adjacencies[i]].key;
		LabelNeighs_NodeLabel outLabel = UNDEFINED_NODE; // TODO: Get label
		if( outLabel == CRITICAL_NODE /*|| type == UNDEFINED_NODE*/ ) {
			if( !receivedCopyFrom(msg_item->copies, neigh_id) )
				missed++;

			critical_neighs++;
		}
	}
	return ((double)missed) / critical_neighs;
}
