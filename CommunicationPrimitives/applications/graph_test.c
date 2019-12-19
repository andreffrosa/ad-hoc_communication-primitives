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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uuid/uuid.h>

#include "utility/myUtils.h"
#include "data_structures/graph.h"

static bool equalID(uuid_t a, uuid_t b) {
	return uuid_compare(a, b) == 0;
}

int main(int argc, char* argv[]) {
	int size = 10;
	graph* graph = graph_init_capacity(size, (comparator_function) &equalID);
	graph_set_delete_item(graph, NULL);
	assert(graph->capacity == size);
	assert(graph->size == 0);
	assert(graph->nodes != NULL);

	// Insert
	uuid_t key1;
	uuid_generate_random(key1);
	void* result = graph_insert(graph, key1, key1, NULL, false);
	assert(graph->size == 1);
	assert(result == NULL);
	assert(graph->nodes[0].degree == 0);
	assert(graph->nodes[0].adjacencies == NULL);
	assert(equalID(graph->nodes[0].key, key1));
	assert(equalID(graph->nodes[0].value, key1));

	list* l = list_init();
	list_add_item_to_tail(l, key1);

	uuid_t key2;
	uuid_generate_random(key2);
	result = graph_insert(graph, key2, key2, l, false);
	assert(graph->size == 2);
	assert(result == NULL);
	assert(graph->nodes[1].degree == 1);
	assert(graph->nodes[1].adjacencies != NULL);
	assert(equalID(graph->nodes[1].key, key2));
	assert(equalID(graph->nodes[1].value, key2));

	assert(graph->nodes[0].degree == 1);
	assert(graph->nodes[0].adjacencies != NULL);

	assert(graph->nodes[0].adjacencies[0] == 1);
	assert(graph->nodes[1].adjacencies[0] == 0);

	uuid_t key3;
	uuid_generate_random(key3);

	l = list_init();
	list_add_item_to_tail(l, key1);
	list_add_item_to_tail(l, key2);

	result = graph_insert(graph, key3, key3, l, false);
	assert(graph->size == 3);
	assert(result == NULL);
	assert(graph->nodes[2].degree == 2);
	assert(graph->nodes[2].adjacencies != NULL);
	assert(equalID(graph->nodes[2].key, key3));
	assert(equalID(graph->nodes[2].value, key3));

	assert(graph->nodes[0].degree == 2);
	assert(graph->nodes[0].adjacencies != NULL);

	assert(graph->nodes[1].degree == 2);
	assert(graph->nodes[1].adjacencies != NULL);

	assert(graph->nodes[0].adjacencies[0] == 1);
	assert(graph->nodes[0].adjacencies[1] == 2);
	assert(graph->nodes[1].adjacencies[0] == 0);
	assert(graph->nodes[1].adjacencies[1] == 2);

	assert(graph->nodes[2].adjacencies[0] == 0);
	assert(graph->nodes[2].adjacencies[1] == 1);

	// Adjacencies
	list* a = graph_get_adjacencies(graph, key1, sizeof(uuid_t));
	unsigned char* x = (unsigned char*) list_remove_head(a);
	assert(equalID(x, key2));
	x = (unsigned char*) list_remove_head(a);
	assert(equalID(x, key3));


	/*	result = graph_insert(graph, key1, key1, NULL, false);
	assert(result != NULL);
	assert(equalID(key1, result));
	 */

	// Find
	graph_item* it = graph_find_item(graph, key1);
	assert(it != NULL);
	assert(equalID(key1, it->key));
	assert(equalID(key1, it->value));

	it = graph_find_item(graph, key2);
	assert(it != NULL);
	assert(equalID(key2, it->key));
	assert(equalID(key2, it->value));

	it = graph_find_item(graph, key3);
	assert(it != NULL);
	assert(equalID(key3, it->key));
	assert(equalID(key3, it->value));

	uuid_t key4;
	uuid_generate_random(key4);
	it = graph_find_item(graph, key4);
	assert(it == NULL);

	// Remove
	void* v = graph_remove(graph, key1);
	assert(v!= NULL);
	assert(graph->size == 2);

	v = graph_remove(graph, key2);
	assert(v!= NULL);
	assert(graph->size == 1);

	v = graph_remove(graph, key3);
	assert(v!= NULL);
	assert(graph->size == 0);

	// Resize
	int old_size = graph->capacity;
	int n_keys = 30;
	uuid_t keys[n_keys];
	for(int i = 0; i < n_keys; i++) {
		uuid_generate_random(keys[i]);
		graph_insert(graph, keys[i], keys[i], NULL, false);
	}

	assert(graph->capacity > old_size);

	graph_delete(graph);

	/////////

	graph = graph_init_capacity(size, (comparator_function) &equalID);

	assert(graph->size == 0);

	graph_insert_node(graph, key1, key1);
	assert(graph->size == 1);
	graph_item* node1 = graph_find_item(graph, key1);
	assert(node1 != NULL);
	assert(node1->adjacencies == NULL);
	assert(node1->degree == 0);

	graph_insert_node(graph, key2, key2);
	assert(graph->size == 2);
	graph_item* node2 = graph_find_item(graph, key2);
	assert(node2 != NULL);
	assert(node2->adjacencies == NULL);
	assert(node2->degree == 0);

	graph_insert_node(graph, key3, key3);
	assert(graph->size == 3);
	graph_item* node3 = graph_find_item(graph, key3);
	assert(node3 != NULL);
	assert(node3->adjacencies == NULL);
	assert(node3->degree == 0);

	int r = graph_insert_edge(graph, key1, key2);
	assert(r == 1);
	assert(node1->adjacencies != NULL);
	assert(node1->degree == 1);
	assert(node2->adjacencies != NULL);
	assert(node2->degree == 1);
	assert(equalID(graph->nodes[node1->adjacencies[0]].key, key2));
	assert(equalID(graph->nodes[node2->adjacencies[0]].key, key1));

	r = graph_insert_edge(graph, key1, key2);
	assert(r == 0);
	assert(node1->adjacencies != NULL);
	assert(node1->degree == 1);
	assert(node2->adjacencies != NULL);
	assert(node2->degree == 1);
	assert(equalID(graph->nodes[node1->adjacencies[0]].key, key2));
	assert(equalID(graph->nodes[node2->adjacencies[0]].key, key1));

	r = graph_insert_edge(graph, key1, key4);
	assert(r == -1);
	assert(node1->adjacencies != NULL);
	assert(node1->degree == 1);
	assert(equalID(graph->nodes[node1->adjacencies[0]].key, key2));

	r = graph_insert_edge(graph, key1, key3);
	assert(r == 1);
	assert(node1->adjacencies != NULL);
	assert(node1->degree == 2);
	assert(node3->adjacencies != NULL);
	assert(node3->degree == 1);
	assert(equalID(graph->nodes[node1->adjacencies[1]].key, key3));
	assert(equalID(graph->nodes[node3->adjacencies[0]].key, key1));

	return 0;
}
