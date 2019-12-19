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

#ifndef _MODULES_H_
#define _MODULES_H_

#include <stdarg.h>

#include "messages.h"

typedef struct _RetransmissionContext RetransmissionContext;

struct _RetransmissionContext {
	void* context_args;
	void* context_state;
	void* (*init)(void* context_args, proto_def* protocol_definition);
	void (*process_event)(void* context_args, void* context_state, queue_t_elem* elem, RetransmissionContext* r_context);
	unsigned int (*create_header)(void* context_args, void* context_state, message_item* msg, void** context_header, RetransmissionContext* r_context);
	bool (*query_handler)(void* context_args, void* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context);
	bool (*query_header_handler)(void* context_args, void* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context);
};

bool query_context(RetransmissionContext* r_context, char* query, void* result, int argc, ...);
bool query_context_header(RetransmissionContext* r_context, void* header, unsigned int length, char* query, void* result, int argc, ...);

typedef struct _RetransmissionDelay {
	void* args;
	void* state;
	unsigned long (*r_delay)(void* args, void* state, message_item* msg, RetransmissionContext* r_context);
} RetransmissionDelay;

typedef struct _RetransmissionPolicy {
	void* args;
	void* state;
	bool (*r_policy)(void* args, void* state, message_item* msg, RetransmissionContext* r_context);
} RetransmissionPolicy;

typedef struct _BroadcastAlgorithm {
	RetransmissionDelay r_delay;
	RetransmissionPolicy r_policy;
	RetransmissionContext r_context;
	unsigned int n_phases;
} BroadcastAlgorithm;

#endif /* _MODULES_H_ */
