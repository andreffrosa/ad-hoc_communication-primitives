/*
 * ordered_list.c
 *
 *  Created on: Sept 19, 2019
 *      Author: akos & André Rosa
 */


#include "double_list.h"

void double_list_init_(double_list* l) {
	l->head = NULL;
	l->tail = NULL;
	l->size = 0;
}

double_list* double_list_init() {
	double_list* l = malloc(sizeof(double_list));
	l->head = NULL;
	l->tail = NULL;
	l->size = 0;

	return l;
}

void double_list_add_item_to_head(double_list* l, void* item) {
	double_list_item* new_item = malloc(sizeof(double_list_item));
	new_item->data = item;
	new_item->next = l->head;
	new_item->prev = NULL;
	l->head = new_item;

	if(l->size == 0) {
		l->tail = new_item;
	} else {
		l->head->next->prev = new_item;
	}

	l->size ++;
}

void double_list_add_item_to_tail(double_list* l, void* item) {
	double_list_item* new_item = malloc(sizeof(double_list_item));
	new_item->data = item;
	new_item->next = NULL;
	new_item->prev = l->tail;

	if(l->size == 0) {
		l->head = new_item;
	} else {
		l->tail->next = new_item;
	}

	l->tail = new_item;
	l->size ++;
}

double_list_item* double_list_find_item(double_list* l, comparator_function equal, void* to_find) {
	double_list_item* it = l->head;

	while(it) {
		if(equal(it->data, to_find)) {
			return it;
		}
		it = it->next;
	}

	return NULL;
}

void* double_list_find(double_list* l, comparator_function equal, void* to_find) {
	double_list_item* it = double_list_find_item(l, equal, to_find);
	if(it)
		return it->data;
	else
		return NULL;
}

void* double_list_remove_item(double_list* l, double_list_item* item) {
	void* data = NULL;

	if(item != NULL) {
		double_list_item* prev = item->prev;
		double_list_item* next = item->next;
		data = item->data;

		if(prev != NULL && next != NULL) { //middle
			prev->next = next;
			next->prev = prev;
		} else if(next != NULL && prev == NULL) { //head
			next->prev = NULL;
			l->head = next;
		} else if(prev != NULL && next == NULL) { //tail
			prev->next = NULL;
			l->tail = prev;
		} else { //1 element
			l->head = NULL;
			l->tail = NULL;
		}

		item->next = NULL;
		item->prev = NULL;

		free(item);
		l->size --;
	}

	return data;
}

void* double_list_remove(double_list* l, comparator_function equal, void* to_remove) {
	double_list_item* it = double_list_find_item(l, equal, to_remove);
	if(it)
		return double_list_remove_item(l, it);
	else
		return NULL;
}

void* double_list_get_item_by_index(double_list* l, int index) {
	void* data = NULL;

	int i = 0;
	double_list_item* it = l->head;
	for(; it != NULL && i < index; it = it->next, i++);

	if(it)
		data = it->data;

	return data;
}

void* double_list_remove_head(double_list* l) {
	return double_list_remove_item(l, l->head);
}

void* double_list_remove_tail(double_list* l) {
	return double_list_remove_item(l, l->tail);
}

void* double_list_update_item(double_list* l, comparator_function equal, void* newdata) {
	void* old_data = NULL;

	double_list_item* it = double_list_find_item(l, equal, newdata);

	if( it != NULL ) {
		old_data = it->data;
		it->data = newdata;
	} else
		double_list_add_item_to_head(l, newdata);

	return old_data;
}
