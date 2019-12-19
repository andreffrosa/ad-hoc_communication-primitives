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

#ifndef DATA_STRUCTURES_DOUBLE_LIST_H_
#define DATA_STRUCTURES_DOUBLE_LIST_H_

#include "list.h"

#include <stdlib.h>

typedef struct __double_list_item {
	void* data; //a generic pointer to the data type stored in the list
	struct __double_list_item* next; //a pointer to the next element in the list (NULL if this item is the last element)

	struct __double_list_item* prev; //a pointer to the previous element in the list (NULL if this item is the first element)
} double_list_item;

typedef struct _double_list {
	short size; //number of elements in the list
	double_list_item* head; //pointer to the head of the list
	double_list_item* tail; //pointer to the tail of the list
} double_list;


/**
 * Initialize an empty double list
 * @return a pointer to a an empty double list
 */
double_list* double_list_init();

void double_list_init_(double_list*);

/**
 * Add an item to the head of the double list
 * @param l the ordered list to be added to
 * @param item the item to be added
 */
void double_list_add_item_to_head(double_list* l, void* item);

/**
 * Add an item to the tail of the double list
 * @param l the ordered list to be added to
 * @param item the item to be added
 */
void double_list_add_item_to_tail(double_list* l, void* item);

/**
 * Remove the item pointed by item from the ordered list
 * @param l the ordered list to be removed from
 * @param item the item to be removed
 * @return the removed item data or NULL if no item was removed
 */
void* double_list_remove_item(double_list* l, double_list_item* item);

/**
 * Find a specific item from the double list
 * @param l the double list
 * @param equal a function to compare items in the list (exact match boolean)
 * @param to_find the item to be found
 * @return the found item data or NULL if no item was found
 */
void* double_list_find(double_list* l, comparator_function equal, void* to_find);

double_list_item* double_list_find_item(double_list* l, comparator_function equal, void* to_find);

/**
 * Return the item in double list that has the given index
 * @param l the double list
 * @param index the index of the desired item
 * @return the retrieved item data or NULL if no item was found
 */
void* double_list_get_item_by_index(double_list* l, int index);

/**
 * Remove from the head of the double list
 * @param l the double list
 * @return the removed item data or NULL if no item was removed
 */
void* double_list_remove_head(double_list* l);

/**
 * Remove from the tail of the double list
 * @param l the double list
 * @return the removed item data or NULL if no item was removed
 */
void* double_list_remove_tail(double_list* l);

/**
 * Remove a specific item from the double list
 * @param l the double list to be removed from
 * @param equal a function to compare items in the list (exact match boolean)
 * @param to_remove the item to be removed
 * @return the removed item data or NULL if no item was removed
 */
void* double_list_remove(double_list* l, comparator_function equal, void* to_remove);

/**
 * Update the item pointed by newdata in the double list.
 * Newdata will be added to list, while the old data will be removed.
 * If newdata does not exist in the double list the item is added to the tail of the double list
 * @param l the double list
 * @param equal a function to compare items in the double list (exact match boolean)
 * @param newdata the data to be updated
 * @return a pointer to the old data or NULL if newdata was not in the list.
 */
void* double_list_update_item(double_list* l, comparator_function equal, void* newdata);

#endif /* DATA_STRUCTURES_DOUBLE_LIST_H_ */
