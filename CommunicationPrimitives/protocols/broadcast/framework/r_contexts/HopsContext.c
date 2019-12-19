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

static void* HopsContextInit(void* context_args, proto_def* protocol_definition) {
	// Do nothing
	return NULL;
}

static void HopsContextEvent(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context) {
	// Do nothing
}

static bool HopsContextQuery(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	return false;
}

static bool HopsContextQueryHeader(void* context_args, void* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {

	if(strcmp(query, "hops") == 0) {
		*((unsigned char*)result) = *((unsigned char*)context_header);
		return true;
	}

	return false;
}

static unsigned int HopsContextHeader(void* context_args, void* context_state, message_item* msg_item, void** context_header, RetransmissionContext* r_context) {
	unsigned int size = sizeof(unsigned char);
	*context_header = malloc(size);

	if(msg_item->copies->size == 0) { // Broadcast Request
		**((unsigned char**)context_header) = 1;
	} else {
		message_copy* msg = (message_copy*)msg_item->copies->head->data;

		unsigned char hops = 0;
		if(!query_context_header(r_context, msg->context_header, msg->header.context_length, "hops", &hops, 0))
			assert(false);

		**((unsigned char**)context_header) = hops + 1;
	}

	return size;
}

RetransmissionContext HopsContext() {
	RetransmissionContext r_context;

	r_context.context_args = NULL;
	r_context.context_state = NULL;

	r_context.init = &HopsContextInit;
	r_context.create_header = &HopsContextHeader;
	r_context.process_event = &HopsContextEvent;
	r_context.query_handler = &HopsContextQuery;
	r_context.query_header_handler = &HopsContextQueryHeader;

	return r_context;
}
