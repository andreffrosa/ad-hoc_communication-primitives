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

#ifndef DATA_STRUCTURES_GRAPH_H_
#define DATA_STRUCTURES_GRAPH_H_

#include "list.h"

typedef struct _graph_item {
	void* key;
	void* value;
	unsigned int* adjacencies;
	unsigned int degree;
} graph_item;

typedef struct _graph {
	graph_item* nodes;
	unsigned int capacity;
	unsigned int size;
	comparator_function cmp;
	void (*delete_item)(graph_item*);
} graph;

graph* graph_init(comparator_function cmp);

void graph_init_(graph* graph, comparator_function cmp);

graph* graph_init_capacity(unsigned int capacity, comparator_function cmp);

void graph_init_capacity_(graph* graph, unsigned int capacity, comparator_function cmp);

graph* graph_init_empty(comparator_function cmp);

void graph_init_empty_(graph* graph, comparator_function cmp);

void graph_set_delete_item(graph* graph, void (*delete_item)(graph_item*));

void graph_resize(graph* graph, unsigned int new_capacity);

void* graph_insert(graph* graph, void* key, void* value, list* adjacencies, bool free_list);

void* graph_insert_node(graph* graph, void* key, void* value);

int graph_insert_edge(graph* graph, void* key1, void* key2);

int graph_find_index(graph* graph, void* key);

graph_item* graph_find_item_from_index(graph* graph, unsigned int index);

graph_item* graph_find_item(graph* graph, void* key);

void* graph_find_value_from_index(graph* graph, unsigned int index);

void* graph_find_value(graph* graph, void* key);

list* graph_get_adjacencies(graph* graph, void* key, unsigned int key_size);

void* graph_remove(graph* graph, void* key);

void* graph_remove_item(graph* graph, graph_item* it);

void graph_delete(graph* graph);

#endif /* DATA_STRUCTURES_GRAPH_H_ */
