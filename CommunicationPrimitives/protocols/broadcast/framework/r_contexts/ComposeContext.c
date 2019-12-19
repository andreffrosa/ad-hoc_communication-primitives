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

#include <stdarg.h>

typedef struct _ComposeContextArgs {
	RetransmissionContext* contexts;
	unsigned int amount;
} ComposeContextArgs;

static void* ComposeContextInit(void* context_args, proto_def* protocol_definition) {
	ComposeContextArgs* args = ((ComposeContextArgs*)context_args);
	for(int i = 0; i < args->amount; i++) {
		args->contexts[i].context_state = args->contexts[i].init(args->contexts[i].context_args, protocol_definition);
	}

	return NULL;
}

static unsigned int ComposeContextHeader(void* context_args, void* context_state, message_item* msg, void** context_header, RetransmissionContext* r_context) {
	*context_header = NULL;

	ComposeContextArgs* args = ((ComposeContextArgs*)context_args);

	unsigned char sizes[args->amount];

	unsigned char buffer[1000];
	unsigned int total_size = sizeof(sizes);

	void* ptr = buffer + sizeof(sizes);
	for(int i = 0; i < args->amount; i++) {
		void* aux_header = NULL;
		sizes[i] = args->contexts[i].create_header(args->contexts[i].context_args, args->contexts[i].context_state, msg, &aux_header, r_context);

		memcpy(ptr, aux_header, sizes[i]);
		free(aux_header);

		total_size += sizes[i];
		ptr += sizes[i];
	}
	memcpy(buffer, sizes, sizeof(sizes));

	*context_header = malloc(total_size);
	memcpy(*context_header, buffer, total_size);

	return total_size;
}

static void ComposeContextEvent(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context) {
	ComposeContextArgs* args = ((ComposeContextArgs*)context_args);
	for(int i = 0; i < args->amount; i++) {
		args->contexts[i].process_event(args->contexts[i].context_args, args->contexts[i].context_state, elem, r_context);
	}
}

static bool ComposeContextQuery(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {

	ComposeContextArgs* args = ((ComposeContextArgs*)context_args);
	for(int i = 0; i < args->amount; i++) {
		bool r = args->contexts[i].query_handler(args->contexts[i].context_args, args->contexts[i].context_state, query, result, argc, argv, r_context);
		if(r) return true;
	}

	return false;
}

static bool ComposeContextQueryHeader(void* context_args, void* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context) {

	ComposeContextArgs* args = ((ComposeContextArgs*)context_args);

	unsigned char sizes[args->amount];
	memcpy(sizes, context_header, sizeof(sizes));

	for(int i = 0; i < args->amount; i++) {
		if(sizes[i] > 0) {
			void* ptr = context_header + i*sizeof(unsigned char);
			bool r = args->contexts[i].query_header_handler(args->contexts[i].context_args, args->contexts[i].context_state, ptr, sizes[i], query, result, argc, argv, r_context);
			if(r) return true;
		}
	}

	return false;
}

RetransmissionContext ComposeContext(int amount, ...) {

	RetransmissionContext* contexts = malloc(amount*sizeof(RetransmissionContext));

	va_list args;
	va_start(args, amount);
	for(int i = 0; i < amount; i++) {
		contexts[i] = va_arg(args, RetransmissionContext);
	}
	va_end(args);

	ComposeContextArgs* c_args = malloc(sizeof(ComposeContextArgs));
	c_args->contexts = contexts;
	c_args->amount = amount;

	RetransmissionContext r_context;

	r_context.context_args = c_args;
	r_context.context_state = NULL;

	r_context.init = &ComposeContextInit;
	r_context.create_header = &ComposeContextHeader;
	r_context.process_event = &ComposeContextEvent;
	r_context.query_handler = &ComposeContextQuery;
	r_context.query_header_handler = &ComposeContextQueryHeader;

	return r_context;
}
