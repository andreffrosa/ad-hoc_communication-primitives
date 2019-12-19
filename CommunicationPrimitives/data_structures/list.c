/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * André Rosa (af.rosa@campus.fct.unl.pt)
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2019
 *********************************************************/

#include "list.h"

void list_append(list* l1, list* l2) {
	l1->tail->next = l2->head;
	l1->tail = l2->tail;
	l1->size += l2->size;

	free(l2);
}

void delete_list(list* l) {
	list_item* it = NULL;
	while( (it = list_remove_head(l)) )
		free(it);
	free(l);
}
