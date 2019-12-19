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

#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_

#include "data_structures/double_list.h"

#include "messages.h"
#include "bcast_header.h"
#include "modules.h"

#include "bcast_algorithms.h"

#define BCAST_FRAMEWORK_PROTO_ID 160
#define BCAST_FRAMEWORK_PROTO_NAME "Broadcast Framework"

typedef enum {
	FRAMEWORK_BCAST_MSG_REQ = 0,
	FRAMEWORK_STATS_REQ = 1
} bcast_requests;

typedef struct _bcast_stats {
	unsigned int messages_sent;
	unsigned int messages_not_sent;
	unsigned int messages_delivered;
	unsigned int messages_received;
	unsigned int messages_bcasted;
} bcast_stats;

typedef struct _framework_args {
	BroadcastAlgorithm algorithm;
	unsigned long seen_expiration_ms;
	unsigned long gc_interval_s;
} framework_args;

proto_def* framework_init(void* arg);
void* framework_main_loop(main_loop_args* args);

typedef enum {
	MESSAGE_TIMEOUT = 1,
	TIMER_TYPE_COUNT = 2
} TimerType;

typedef enum {
	BROADCAST_MESSAGE = 1,
	MESSAGE_TYPE_COUNT = 2
} MessageType;

short popMessageType(YggMessage* msg);
void pushMessageType(YggMessage* msg, short type);

#endif /* _FRAMEWORK_H_ */
