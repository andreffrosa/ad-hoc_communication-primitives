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

static void* EmptyContextInit(void* context_args, proto_def* protocol_definition) {
	// Do nothing
	return NULL;
}

static unsigned int EmptyContextHeader(void* context_args, void* context_state, message_item* msg, void** context_header, RetransmissionContext* r_context) {
	*context_header = NULL;
	return 0;
}

static void EmptyContextEvent(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context) {
	// Do nothing
}

static bool EmptyContextQuery(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	return false;
}

static bool EmptyContextQueryHeader(void* context_args, void* context_state, void* header, unsigned int header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {
	return false;
}

RetransmissionContext EmptyContext() {
	RetransmissionContext r_context;

	r_context.context_args = NULL;
	r_context.context_state = NULL;

	r_context.init = &EmptyContextInit;
	r_context.create_header = &EmptyContextHeader;
	r_context.process_event = &EmptyContextEvent;
	r_context.query_handler = &EmptyContextQuery;
	r_context.query_header_handler = &EmptyContextQueryHeader;

	return r_context;
}
