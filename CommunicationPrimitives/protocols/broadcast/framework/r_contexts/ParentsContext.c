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

#include <assert.h>

#include "utility/myUtils.h"

static void* ParentsContextInit(void* context_args, proto_def* protocol_definition) {
	// Do nothing
	return NULL;
}

static void ParentsContextEvent(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context) {
	// Do nothing
}

static bool ParentsContextQuery(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	return false;
}

static bool ParentsContextQueryHeader(void* context_args, void* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	unsigned int amount = context_header_size / sizeof(uuid_t);

	if(strcmp(query, "parents") == 0) {
		list* l = *((list**)result) = list_init();

		for(int i = 0; i < amount; i++) {
			unsigned char* id = malloc(sizeof(uuid_t));
			unsigned char* ptr = context_header + i*sizeof(uuid_t);
			uuid_copy(id, ptr);
			list_add_item_to_tail(l, id);
		}

		return true;
	}

	return false;
}

static unsigned int ParentsContextHeader(void* context_args, void* context_state, message_item* msg_item, void** context_header, RetransmissionContext* r_context) {
	unsigned int max_amount = *((unsigned int*)context_args);
	unsigned int real_amount = iMin(max_amount, msg_item->copies->size);
	unsigned int size = real_amount*sizeof(uuid_t);

	*context_header = malloc(size);

	double_list_item* it = msg_item->copies->head;
	for(int i = 0; i < real_amount; i++, it = it->next) {
		message_copy* copy = (message_copy*)it->data;
		unsigned char* ptr = (unsigned char*)(*context_header + i*sizeof(uuid_t));
		uuid_copy(ptr, copy->header.sender_id);
	}

	return size;
}

RetransmissionContext ParentsContext(unsigned int max_amount) {
	RetransmissionContext r_context;
	r_context.context_args = malloc(sizeof(unsigned int));
	*((unsigned int*)r_context.context_args) = max_amount;
	r_context.context_state = NULL;

	r_context.init = &ParentsContextInit;
	r_context.create_header = &ParentsContextHeader;
	r_context.process_event = &ParentsContextEvent;
	r_context.query_handler = &ParentsContextQuery;
	r_context.query_header_handler = &ParentsContextQueryHeader;

	return r_context;
}
