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

#include "r_policies.h"

#include <assert.h>

typedef struct _EnsemblePolicyArgs {
	int amount;
	bool (*ensemble_function)(bool* values, int amount);
} EnsemblePolicyArgs;

static bool _EnsemblePolicy(void* args, void* state, message_item* msg, RetransmissionContext* r_context) {
	RetransmissionPolicy* policies = ((RetransmissionPolicy*)state);
	EnsemblePolicyArgs* _args = ((EnsemblePolicyArgs*)args);


	bool values[_args->amount];
	for(int i = 0; i < _args->amount; i++) {
		values[i] = policies[i].r_policy(policies[i].args, policies[i].state, msg, r_context);
	}

	return _args->ensemble_function(values, _args->amount);
}

RetransmissionPolicy EnsemblePolicy(bool (*ensemble_function)(bool* values, int amount), int amount, ...) {

	assert(amount >= 1);
	assert( amount == 1 || ensemble_function != NULL);

	RetransmissionPolicy r_policy;

	RetransmissionPolicy* policies = malloc(amount*sizeof(RetransmissionPolicy));
	va_list p_args;
	va_start(p_args, amount);
	for(int i = 0; i < amount; i++) {
		policies[i] = va_arg(p_args, RetransmissionPolicy);
	}
	va_end(p_args);
	r_policy.state = policies;

	EnsemblePolicyArgs* args = malloc(sizeof(EnsemblePolicyArgs));
	args->amount = amount;
	args->ensemble_function = ensemble_function;
	r_policy.args = args;

	r_policy.r_policy = &_EnsemblePolicy;

	return r_policy;
}
