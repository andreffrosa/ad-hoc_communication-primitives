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

#include "protocols/discovery/topology_discovery.h"
#include "data_structures/graph.h"
#include "utility/myUtils.h"

#include <assert.h>

typedef struct _NeighborsContextState {
	graph neighborhood;
	uuid_t myID;
} NeighborsContextState;

typedef struct _NeighborsContextArgs {
	topology_discovery_args* d_args;
	bool append_neighbors;
} NeighborsContextArgs;

static bool equalID(uuid_t a, uuid_t b) {
	return uuid_compare(a, b) == 0;
}

static unsigned int minMaxNeighs(graph* neighborhood, unsigned char* myID, bool max);
static void getNeighboursDistribution(graph* neighborhood, unsigned char* myID, double* result);
static list* getNeighboursInCommon(graph* neighborhood, uuid_t neigh1_id, uuid_t neigh2_id);
static list* getCoverage(graph* neighborhood, unsigned char* myID, double_list* copies);
static list* notCovered(graph* neighborhood, unsigned char* myID, double_list* copies);
static bool allCovered(graph* neighborhood, unsigned char* myID, double_list* copies);

struct announce_state {
	graph* graph;
};

static void process_node(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id, unsigned char n_neighs) {
	graph* graph = ((struct announce_state*)state)->graph;

	unsigned char* _id = malloc(sizeof(uuid_t));
	uuid_copy(_id, id);

	graph_insert_node(graph, _id, NULL);
}

static void process_edge(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id1, unsigned char* id2) {
	graph* graph = ((struct announce_state*)state)->graph;

	graph_insert_edge(graph, id1, id2);
}

static void createGraph(void* context_args, void* context_state, void* announce, int size) {
	NeighborsContextState* state = (NeighborsContextState*) context_state;
	//NeighborsContextArgs* args = (NeighborsContextArgs*) context_args;

	graph_delete(&state->neighborhood);

	graph_init_(&state->neighborhood, (comparator_function) &equalID);
	graph_insert_node(&state->neighborhood, state->myID, NULL);

	struct announce_state _state = {
			.graph = &state->neighborhood
	};
	processAnnounce(&_state, announce, size, -1, &process_node, &process_edge);
}


static void* NeighborsContextInit(void* context_args, proto_def* protocol_definition) {
	// Register Topology Discovery Protocol
	registerProtocol(TOPOLOGY_DISCOVERY_PROTOCOL, &topology_discovery_init, ((NeighborsContextArgs*)context_args)->d_args);
	proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBOR_FOUND);
	proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBOR_UPDATE);
	proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBOR_LOST);

	NeighborsContextState* state = malloc(sizeof(NeighborsContextState));

	getmyId(state->myID);

	graph_init_(&state->neighborhood, (comparator_function) &equalID);
	graph_insert_node(&state->neighborhood, state->myID, NULL);

	return state;
}

static unsigned int NeighborsContextHeader(void* context_args, void* context_state, message_item* msg, void** context_header, RetransmissionContext* r_context) {
	NeighborsContextState* state = (NeighborsContextState*) context_state;
	NeighborsContextArgs* args = (NeighborsContextArgs*)context_args;

	if(args->append_neighbors) {
		unsigned int size = sizeof(unsigned char);
		*context_header = malloc(size);

		graph_item* node = graph_find_item(&state->neighborhood, state->myID);
		assert(node != NULL);

		*((unsigned int*)context_header) = node->degree;
		return size;
	}

	*context_header = NULL;
	return 0;
}

static void NeighborsContextEvent(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context) {
	if(elem->type == YGG_EVENT) {
		/*if(elem->data.event.notification_id == NEIGHBOR_FOUND) {
			// TODO:
		} else if(elem->data.event.notification_id == NEIGHBOR_UPDATE) {
			// TODO:
		} else if(elem->data.event.notification_id == NEIGHBOR_LOST) {
			// TODO:
		}*/
		if(elem->data.event.notification_id == NEIGHBORHOOD_UPDATE) {
			createGraph(context_args, context_state, elem->data.event.payload, elem->data.event.length);
		}
	}
}

static bool NeighborsContextQuery(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	NeighborsContextState* state = (NeighborsContextState*) context_state;

	if(strcmp(query, "graph") == 0) {
		*((graph**)result) = &state->neighborhood;
		return true;
	} else if(strcmp(query, "my_id") == 0) {
		uuid_copy(((unsigned char*)result), state->myID);
		return true;
	} else if(strcmp(query, "n_neighbors") == 0) {
		graph_item* node = graph_find_item(&state->neighborhood, state->myID);
		assert(node != NULL);

		if(node->degree == 0 && node->value != NULL)
			*((unsigned int*)result) = *((unsigned char*)node->value);
		else
			*((unsigned int*)result) = node->degree;

		return true;
	} else if(strcmp(query, "max_neighbors") == 0) {
		*((unsigned int*)result) = minMaxNeighs(&state->neighborhood, state->myID, true);
		return true;
	} else if(strcmp(query, "min_neighbors") == 0) {
		*((unsigned int*)result) = minMaxNeighs(&state->neighborhood, state->myID, false);
		return true;
	} else if(strcmp(query, "neighbors_distribution") == 0) {
		getNeighboursDistribution(&state->neighborhood, state->myID, (double*)result);
		return true;
	} else if(strcmp(query, "coverage") == 0) {
		assert(argc >= 1);
		message_item* msg_item = va_arg(*argv, message_item*);
		*((list**)result) = getCoverage(&state->neighborhood, state->myID, msg_item->copies);
		return true;
	} else if(strcmp(query, "not_covered") == 0) {
		assert(argc >= 1);
		message_item* msg_item = va_arg(*argv, message_item*);
		*((list**)result) = notCovered(&state->neighborhood, state->myID, msg_item->copies);
		return true;
	} else if(strcmp(query, "all_covered") == 0) {
		assert(argc >= 1);
		message_item* msg_item = va_arg(*argv, message_item*);
		*((bool*)result) = allCovered(&state->neighborhood, state->myID, msg_item->copies);
		return true;
	} else {
		return false;
	}
}

static bool NeighborsContextQueryHeader(void* context_args, void* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	if(strcmp(query, "neighbors") == 0) {
		*((unsigned int*)result) = *((unsigned int*)context_header);
		return true;
	}
	return false;
}

RetransmissionContext NeighborsContext(topology_discovery_args* d_args, bool append_neighbors) {
	RetransmissionContext r_context;

	NeighborsContextArgs* args = malloc(sizeof(NeighborsContextArgs));
	args->d_args = d_args;
	args->append_neighbors = append_neighbors;

	r_context.context_args = args;
	r_context.context_state = NULL;

	r_context.init = &NeighborsContextInit;
	r_context.create_header = &NeighborsContextHeader;
	r_context.process_event = &NeighborsContextEvent;
	r_context.query_handler = &NeighborsContextQuery;
	r_context.query_header_handler = &NeighborsContextQueryHeader;

	return r_context;
}

static unsigned int minMaxNeighs(graph* neighborhood, unsigned char* myID, bool max) {
	graph_item* node = graph_find_item(neighborhood, myID);
	assert(node != NULL);

	unsigned int value = 0;
	for(int i = 0; i < node->degree; i++) {
		if(max)
			value = iMax(value, neighborhood->nodes[node->adjacencies[i]].degree);
		else
			value = iMin(value, neighborhood->nodes[node->adjacencies[i]].degree);
	}

	return value;
}

// TODO: passar estas ops para a biblioteca de grafos?
static list* getNeighboursInCommon(graph* neighborhood, uuid_t neigh1_id, uuid_t neigh2_id) {
	assert(neighborhood != NULL);

	list* list = list_init();

	graph_item* node1 = graph_find_item(neighborhood, neigh1_id);
	graph_item* node2 = graph_find_item(neighborhood, neigh2_id);

	if(node1 == NULL || node2 == NULL) {
		return list;
	}

	for(int i = 0; i < node1->degree; i++) {
		for(int j = 0; j < node2->degree; j++) {
			if(node1->adjacencies[i] == node2->adjacencies[j]) {
				unsigned char* id = malloc(sizeof(uuid_t));
				uuid_copy(id, neighborhood->nodes[node1->adjacencies[i]].key);
				list_add_item_to_head(list, id);
			}
		}
	}

	return list;
}

static list* getCoverage(graph* neighborhood, unsigned char* myID, double_list* copies) {
	list* total_coverage = list_init();

	for(double_list_item* it = copies->head; it; it = it->next) {
		message_copy* msg_copy = (message_copy*)it->data;

		list* neigh_coverage = getNeighboursInCommon(neighborhood, myID, msg_copy->header.sender_id);

		list_append(total_coverage, neigh_coverage);
	}

	return total_coverage;
}

static list* notCovered(graph* neighborhood, unsigned char* myID, double_list* copies) {
	list* missed = list_init();

	list* total_coverage = getCoverage(neighborhood, myID, copies);

	graph_item* node = graph_find_item(neighborhood, myID);

	for(int i = 0; i < node->degree; i++) {
		unsigned char* neigh_id = neighborhood->nodes[node->adjacencies[i]].key;

		unsigned char* aux = list_remove_item(total_coverage, (comparator_function)&equalID, neigh_id);
		if( aux != NULL ) {
			free(aux);
		} else {
			unsigned char* id = malloc(sizeof(uuid_t));
			uuid_copy(id, neigh_id);
			list_add_item_to_tail(missed, id);
		}
	}
	delete_list(total_coverage);

	return missed;
}

static bool allCovered(graph* neighborhood, unsigned char* myID, double_list* copies) {
	list* missed = notCovered(neighborhood, myID, copies);
	bool all_covered = missed->size == 0;
	delete_list(missed);
	return all_covered;
}

static void getNeighboursDistribution(graph* neighborhood, unsigned char* myID, double* result) {
	unsigned int max = minMaxNeighs(neighborhood, myID, true);
	graph_item* node = graph_find_item(neighborhood, myID);
	assert(node != NULL);
	unsigned int n = node->degree;

	result[0] = n;
	result[1] = 0.0;
	result[2] = 0.0;

	if( max > 0 && n > 0 ) {
		int* dist = malloc((max+1)*sizeof(int));
		bzero(dist, (max+1)*sizeof(int));

		int nn = node->degree, sum = node->degree;
		dist[nn]++;

		for(int i = 0; i < node->degree; i++) {
			nn = neighborhood->nodes[node->adjacencies[i]].degree;
			dist[nn]++;
			sum += nn;
		}

		if( sum > 0 && n > 0 ) {
			double p=0.0, cdf=0.0;
			for(int i = max-1; i >= n; i--) {
				p =  ((double) dist[i]) / sum;
				cdf = p + cdf;
			}
			result[1] = p;
			result[2] = cdf;
		}
		free(dist);
	}
}

