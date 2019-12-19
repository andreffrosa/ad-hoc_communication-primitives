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

#include "hash_table.h"

#define DEFAULT_SIZE 10
#define UPPER_RATIO 75
#define LOWER_RATIO 10

static bool equals_hash_table_item(hash_table_item* it, void* key) {
	return it->cmp(it->key, key);
}

hash_table* hash_table_init_size(unsigned int size, hashing_function hash_fun, comparator_function comp_fun) {
	unsigned int prime = isPrime(size) ? size : nextPrime(size);

	hash_table* table = malloc(sizeof(hash_table));
	table->array = malloc(prime * sizeof(double_list*));

	for (int i = 0; i < prime; i++) {
		table->array[i] = double_list_init();
	}

	table->array_size = prime;
	table->n_items = 0;
	table->hash_fun = hash_fun;
	table->comp_fun = comp_fun;

	return table;
}

hash_table* hash_table_init(hashing_function hash_fun, comparator_function comp_fun) {
	return hash_table_init_size(DEFAULT_SIZE, hash_fun, comp_fun);
}

static void resize(hash_table* table, unsigned int new_size) {
	unsigned int new_size_ = nextPrime(new_size);

	double_list** new_array = malloc(new_size_ * sizeof(double_list));

	for (int i = 0; i < new_size_; i++) {
		new_array[i] = double_list_init();
	}

	for (int i = 0; i < table->array_size; i++) {
		hash_table_item* item = NULL;
		while ((item = (hash_table_item*) double_list_remove_head(table->array[i]))) {
			unsigned int index = table->hash_fun(item->key) % new_size_;
			double_list_add_item_to_tail(new_array[index], item);
		}

		free(table->array[i]);
	}
	free(table->array);

	table->array = new_array;
	table->array_size = new_size_;
}

hash_table_item* hash_table_find_item(hash_table* table, void* key) {
	unsigned int index = table->hash_fun(key) % table->array_size;
	return (hash_table_item*) double_list_find(table->array[index], (comparator_function) &equals_hash_table_item, key);
}

void* hash_table_find_value(hash_table* table, void* key) {
	hash_table_item* it = hash_table_find_item(table, key);
	if (it != NULL)
		return it->value;
	else
		return NULL;
}

void* hash_table_insert(hash_table* table, void* key, void* value) {
	int ratio = (table->n_items + 1) * 100 / table->array_size;
	if (ratio >= UPPER_RATIO)
		resize(table, table->array_size*2);

	unsigned int index = table->hash_fun(key) % table->array_size;
	hash_table_item* item = (hash_table_item*) double_list_find(table->array[index], (comparator_function) &equals_hash_table_item, key);
	if (item == NULL) {
		item = malloc(sizeof(hash_table_item));
		item->key = key;
		item->value = value;
		item->cmp = table->comp_fun;
		double_list_add_item_to_tail(table->array[index], item);
		table->n_items++;
		return NULL;
	} else {
		void* aux = item->value;
		item->value = value;
		return aux;
	}
}

void* hash_table_remove(hash_table* table, void* key) {
	void* value = NULL;

	unsigned int index = table->hash_fun(key) % table->array_size;
	double_list_item* it = double_list_find_item(table->array[index], (comparator_function) &equals_hash_table_item, key);
	if (it != NULL) {
		hash_table_item* item = (hash_table_item*) double_list_remove_item(table->array[index], it);
		table->n_items--;

		value = item->value;
		free(item);
	}

	/*int ratio = (table->n_items) * 100 / table->array_size;
	if (ratio <= LOWER_RATIO )
		resize(table, table->array_size/2);*/

	return value;
}

static void free_hash_table_item(hash_table_item* current) {
	if(current->key == current->value && current->key != NULL)
		free(current->key);
	else {
		if(current->key != NULL)
			free(current->key);
		if(current->value != NULL)
			free(current->value);
	}
}

void hash_table_delete(hash_table* table) {
	hash_table_delete_custom(table, &free_hash_table_item);
}

void hash_table_delete_custom(hash_table* table, void (*delete_item)(hash_table_item*)) {

	for(int i = 0; i < table->array_size; i++) {
		hash_table_item* current = NULL;
		while( (current = double_list_remove_head(table->array[i])) ) {

			if(delete_item)
				delete_item(current);

			free(current);
		}
		free(table->array[i]);
	}
	free(table->array);

	free(table);
}
