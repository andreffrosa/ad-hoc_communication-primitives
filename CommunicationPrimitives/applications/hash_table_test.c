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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uuid/uuid.h>

#include "../Yggdrasil/Yggdrasil/core/utils/hashfunctions.h"
#include "utility/myUtils.h"
#include "data_structures/hash_table.h"

static unsigned long uuid_hash(uuid_t id) {
	return RSHash(id, sizeof(uuid_t));
}

static bool equalID(uuid_t a, uuid_t b) {
	return uuid_compare(a, b) == 0;
}

int main(int argc, char* argv[]) {

	int size = 10;
	hash_table* table = hash_table_init_size(size, (hashing_function) &uuid_hash, (comparator_function) &equalID);
	assert(table->array_size >= size);
	assert(table->n_items == 0);

	uuid_t key1;
	uuid_generate_random(key1);
	void* v = hash_table_insert(table, key1, key1);
	assert(table->n_items == 1);
	assert(v == NULL);

	uuid_t key2;
	uuid_generate_random(key2);
	v = hash_table_insert(table, key2, key2);
	assert(table->n_items == 2);
	assert(v == NULL);

	uuid_t key3;
	uuid_generate_random(key3);
	v = hash_table_insert(table, key3, key3);
	assert(table->n_items == 3);
	assert(v == NULL);

	//
	v = hash_table_insert(table, key1, key1);
	assert(uuid_compare((unsigned char*) v, key1)==0);

	v = hash_table_find_value(table, key2);
	assert(v != NULL);
	assert(uuid_compare((unsigned char*) v, key2)==0);

	v = hash_table_find_value(table, key3);
	assert(v != NULL);
	assert(uuid_compare((unsigned char*) v, key3)==0);

	uuid_t key4;
	uuid_generate_random(key4);
	v = hash_table_find_value(table, key4);
	assert(v == NULL);

	v = hash_table_insert(table, key1, key4);
	assert(uuid_compare((unsigned char*) v, key1)==0);

	uuid_t key5;
	uuid_generate_random(key5);
	v = hash_table_insert(table, key2, key5);
	assert(uuid_compare((unsigned char*) v, key2)==0);

	uuid_t key6;
	uuid_generate_random(key6);
	v = hash_table_insert(table, key3, key6);
	assert(uuid_compare((unsigned char*) v, key3)==0);

	assert(table->n_items == 3);

	v = hash_table_remove(table, key1);
	assert(uuid_compare((unsigned char*) v, key4)==0);
	assert(table->n_items == 2);

	v = hash_table_remove(table, key2);
	assert(uuid_compare((unsigned char*) v, key5)==0);
	assert(table->n_items == 1);

	v = hash_table_remove(table, key3);
	assert(uuid_compare((unsigned char*) v, key6)==0);
	assert(table->n_items == 0);

	v = hash_table_remove(table, key1);
	assert(v == NULL);
	assert(table->n_items == 0);

	v = hash_table_remove(table, key4);
	assert(v == NULL);
	assert(table->n_items == 0);

	int old_size = table->array_size;
	int n_keys = 30;
	uuid_t keys[n_keys];
	for(int i = 0; i < n_keys; i++) {
		uuid_generate_random(keys[i]);
		hash_table_insert(table, keys[i], keys[i]);
	}

	assert(table->array_size > old_size);

	/*old_size = table->array_size;
	for(int i = 0; i < n_keys; i++) {
		hash_table_remove(table, keys[i]);
	}

	assert(table->array_size < old_size);*/

	hash_table_delete_custom(table, NULL);

	return 0;
}
