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

#include "graph.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define DEFAULT_CAPACITY 10

static void free_graph_item(graph_item* current) {
	if(current->key == current->value && current->key != NULL)
		free(current->key);
	else {
		if(current->key != NULL)
			free(current->key);
		if(current->value != NULL)
			free(current->value);
	}
}

graph* graph_init(comparator_function cmp) {
	return graph_init_capacity(DEFAULT_CAPACITY, cmp);
}

void graph_init_(graph* graph, comparator_function cmp) {
	graph_init_capacity_(graph, DEFAULT_CAPACITY, cmp);
}

graph* graph_init_empty(comparator_function cmp) {
	return graph_init_capacity(0, cmp);
}

void graph_init_empty_(graph* graph, comparator_function cmp) {
	graph_init_capacity_(graph, 0, cmp);
}

graph* graph_init_capacity(unsigned int capacity, comparator_function cmp) {
	graph* graph = malloc(sizeof(struct _graph));
	graph_init_capacity_(graph, capacity, cmp);
	return graph;
}

void graph_init_capacity_(graph* graph, unsigned int capacity, comparator_function cmp) {
	graph->nodes = capacity == 0 ? NULL : malloc(capacity*sizeof(graph_item));
	if(capacity > 0)
		bzero(graph->nodes, capacity*sizeof(graph_item));
	graph->capacity = capacity;
	graph->size = 0;
	graph->cmp = cmp;
	graph->delete_item = &free_graph_item;
}

void graph_set_delete_item(graph* graph, void (*delete_item)(graph_item*)) {
	graph->delete_item = delete_item;
}

void graph_resize(graph* graph, unsigned int new_capacity) {
	assert(new_capacity > graph->capacity);

	graph_item* new_nodes = malloc(new_capacity*sizeof(graph_item));
	bzero(new_nodes, new_capacity*sizeof(graph_item));
	if(graph->nodes != NULL) {
		memcpy(new_nodes, graph->nodes, (graph->capacity)*sizeof(graph_item));
		free(graph->nodes);
	}
	graph->nodes = new_nodes;
	graph->capacity = new_capacity;
}

void* graph_insert(graph* graph, void* key, void* value, list* adjacencies, bool free_list) {
	void* result = NULL;
	graph_item* it = graph_find_item(graph, key);
	if( it != NULL ) {
		result = graph_remove_item(graph, it);
	} else {
		if(graph->size == graph->capacity) {
			graph_resize(graph, graph->capacity == 0 ? DEFAULT_CAPACITY : 2*graph->capacity); // TODO: Or just +1?
		}
	}

	int my_index = graph->size;
	it = &graph->nodes[my_index];
	graph->size++;

	it->key = key;
	it->value = value;
	it->degree = 0;

	if( adjacencies != NULL ) {
		it->adjacencies = malloc(adjacencies->size*sizeof(unsigned int));
		for(list_item* x = list_remove_head(adjacencies); x != NULL; x = list_remove_head(adjacencies)) {
			int index = graph_find_index(graph, x);
			assert(index >= 0);
			it->adjacencies[it->degree++] = index;

			graph_item* neigh = &graph->nodes[index];
			unsigned int* new_adjacencies = malloc(neigh->degree+1);
			memcpy(new_adjacencies, neigh->adjacencies, neigh->degree*sizeof(unsigned int));
			new_adjacencies[neigh->degree++] = my_index;
			free(neigh->adjacencies);
			neigh->adjacencies = new_adjacencies;

			if(free_list)
				free(x);
		}
		free(adjacencies);
	} else {
		it->adjacencies = NULL;
	}

	return result;
}

void* graph_insert_node(graph* graph, void* key, void* value) {
	void* result = NULL;
	graph_item* it = graph_find_item(graph, key);
	if( it != NULL ) {
		result = graph_remove_item(graph, it);
	} else {
		if(graph->size == graph->capacity) {
			graph_resize(graph, graph->capacity == 0 ? DEFAULT_CAPACITY : 2*graph->capacity); // TODO: Or just +1?
		}
	}

	int my_index = graph->size;
	it = &graph->nodes[my_index];
	graph->size++;

	it->key = key;
	it->value = value;
	it->degree = 0;
	it->adjacencies = NULL;

	return result;
}

int graph_insert_edge(graph* graph, void* key1, void* key2) {
	int index1 = graph_find_index(graph, key1);
	int index2 = graph_find_index(graph, key2);

	if(index1 < 0 || index2 < 0)
		return -1;

	graph_item* node1 = &graph->nodes[index1];
	graph_item* node2 = &graph->nodes[index2];

	// Verify if it already exists
	for(int i = 0; i < node1->degree; i++) {
		if(node1->adjacencies[i] == index2)
			return 0;
	}

	unsigned int* node1_new_adj = malloc((node1->degree+1)*sizeof(unsigned int));
	memcpy(node1_new_adj, node1->adjacencies, node1->degree*sizeof(unsigned int));
	node1_new_adj[node1->degree++] = index2;
	if(node1->adjacencies != NULL)
		free(node1->adjacencies);
	node1->adjacencies = node1_new_adj;

	unsigned int* node2_new_adj = malloc((node2->degree+1)*sizeof(unsigned int));
	memcpy(node2_new_adj, node2->adjacencies, node2->degree*sizeof(unsigned int));
	node2_new_adj[node2->degree++] = index1;
	if(node2->adjacencies != NULL)
		free(node2->adjacencies);
	node2->adjacencies = node2_new_adj;

	return 1;
}

graph_item* graph_find_item_from_index(graph* graph, unsigned int index) {
	return &graph->nodes[index];
}

void* graph_find_value_from_index(graph* graph, unsigned int index) {
	return graph->nodes[index].value;
}

int graph_find_index(graph* graph, void* key) {
	for(int i = 0; i < graph->size; i++) {
		if( graph->cmp(graph->nodes[i].key, key) ) {
			return i;
		}
	}
	return -1;
}

graph_item* graph_find_item(graph* graph, void* key) {
	for(int i = 0; i < graph->size; i++) {
		if( graph->cmp(graph->nodes[i].key, key) ) {
			return &graph->nodes[i];
		}
	}
	return NULL;
}

void* graph_find_value(graph* graph, void* key) {
	graph_item* it = graph_find_item(graph, key);
	if(it)
		return it->value;
	else
		return NULL;
}

void* graph_remove_item(graph* graph, graph_item* it) {
	void* result = it->value;
	it->value = NULL;

	if(graph->delete_item)
		graph->delete_item(it);

	int index = graph_find_index(graph, it->key);
	if(graph->size < graph->capacity) {
		int amount = graph->size - index - 1;
		memcpy(graph->nodes + index, graph->nodes + index + 1, amount*sizeof(graph_item));
	}
	bzero(&graph->nodes[graph->size], sizeof(graph_item));
	graph->size--;

	for(int i = 0; i < graph->size; i++) {
		graph_item* current = &graph->nodes[i];
		for(int j = 0; j < current->degree; j++) {
			int cut = -1;
			if(current->adjacencies[j] == index) {
				cut = j;
			} else if(current->adjacencies[j] > index) {
				current->adjacencies[j]--;
			}

			if(cut >= 0) {
				unsigned int* new_adjacencies = NULL;
				if(current->degree > 1) {
					new_adjacencies = malloc((current->degree-1)*sizeof(unsigned int));
					if(cut > 0)
						memcpy(new_adjacencies, current->adjacencies, cut*sizeof(unsigned int));
					if(cut < current->degree - 1)
						memcpy(new_adjacencies + cut, current->adjacencies + cut + 1, (current->degree - cut - 1)*sizeof(unsigned int));
				}
				free(current->adjacencies);
				current->adjacencies = new_adjacencies;
				current->degree--;
			}
		}
	}

	return result;
}

void* graph_remove(graph* graph, void* key) {
	graph_item* it = graph_find_item(graph, key);
	if(it!=NULL){
		return graph_remove_item(graph, it);
	} else {
		return NULL;
	}
}

void graph_delete(graph* graph) {
	for(int i = 0; i < graph->size; i++) {
		graph_item* current = &graph->nodes[i];

		if(graph->delete_item)
			graph->delete_item(current);

		free(current->adjacencies);
	}
	free(graph->nodes);
	free(graph);
}

list* graph_get_adjacencies(graph* graph, void* key, unsigned int key_size) {
	list* l = list_init();

	graph_item* it = graph_find_item(graph, key);
	if(it != NULL) {
		for(int i = 0; i < it->degree; i++) {
			void* aux = malloc(key_size);
			memcpy(aux, graph->nodes[it->adjacencies[i]].key, key_size);
			list_add_item_to_tail(l, aux);
		}
	}

	return l;
}

