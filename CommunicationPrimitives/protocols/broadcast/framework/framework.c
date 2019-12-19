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

#include <assert.h>

#include "utility/myUtils.h"
#include "framework.h"

typedef struct framework_state_ {
	message_list seen_msgs;  		// List of seen messages so far
	uuid_t gc_id;			 		// Garbage Collector timer id
	uuid_t myID;             		// Current process id
	framework_args* args;    		// Framework's arguments
	bcast_stats stats;       		// Framework's stats
	struct timespec current_time;   // Timestamp of the current time
} framework_state;

// Protocol Handlers
static void uponBroadcastRequest(framework_state* state, YggRequest* req);
static void uponNewMessage(framework_state* state, YggMessage* msg);
static void uponTimeout(framework_state* state, YggTimer* timer);

static void init(framework_state* state);
static void ComputeRetransmissionDelay(framework_state* state, message_item* msg_item);
static void changePhase(framework_state* state, message_item* msg);
static void DeliverMessage(framework_state* state, YggMessage* toDeliver);
static void RetransmitMessage(framework_state* state, message_item* msg);
static void uponBroadcastRequest(framework_state* state, YggRequest* req);
static void uponNewMessage(framework_state* state, YggMessage* msg);
static void uponTimeout(framework_state* state, YggTimer* timer);
static void serializeMessage(framework_state* state, YggMessage* m, message_item* msg_item);
static void deserializeMessage(YggMessage* m, bcast_header* header, void** context_header, YggMessage* toDeliver);
static void runGarbageCollector(framework_state* state);
static void uponStatsRequest(framework_state* state, YggRequest* req);

static inline unsigned long triggerRetransmissionDelay(framework_state* state, message_item* msg_item) {
	return state->args->algorithm.r_delay.r_delay(state->args->algorithm.r_delay.args, state->args->algorithm.r_delay.state, msg_item, &state->args->algorithm.r_context);
}

static inline bool triggerRetransmissionPolicy(framework_state* state, message_item* msg_item) {
	return state->args->algorithm.r_policy.r_policy(state->args->algorithm.r_policy.args, state->args->algorithm.r_policy.state, msg_item, &state->args->algorithm.r_context);
}

static inline unsigned int triggerRetransmissionContextHeader(framework_state* state, message_item* msg_item, void** context_header) {
	return state->args->algorithm.r_context.create_header(state->args->algorithm.r_context.context_args, state->args->algorithm.r_context.context_state, msg_item, context_header, &state->args->algorithm.r_context);
}

static inline void triggerRetransmissionContextInit(framework_state* state, proto_def* protocol_definition) {
	state->args->algorithm.r_context.context_state = state->args->algorithm.r_context.init(state->args->algorithm.r_context.context_args, protocol_definition);
}

static inline void triggerRetransmissionContextEvent(framework_state* state, queue_t_elem* event) {
	return state->args->algorithm.r_context.process_event(state->args->algorithm.r_context.context_args, state->args->algorithm.r_context.context_state, event, &state->args->algorithm.r_context);
}

bool query_context(RetransmissionContext* r_context, char* query, void* result, int argc, ...) {
	va_list args;
	va_start(args, argc);
	bool r = r_context->query_handler(r_context->context_args, r_context->context_state, query, result, argc, &args, r_context);
	va_end(args);
	return r;
}

bool query_context_header(RetransmissionContext* r_context, void* context_header, unsigned int context_length, char* query, void* result, int argc, ...) {
	va_list args;
	va_start(args, argc);
	bool r = r_context->query_header_handler(r_context->context_args, r_context->context_state, context_header, context_length, query, result, argc, &args, r_context);
	va_end(args);
	return r;
}

void* framework_main_loop(main_loop_args* args) {

	framework_state* state = args->state;

	queue_t_elem elem;
	while (1) {
		// Retrieve an event from the queue
		queue_pop(args->inBox, &elem);

		clock_gettime(CLOCK_REALTIME, &state->current_time); // Update current time

		switch (elem.type) {
		case YGG_TIMER:
			if( uuid_compare(elem.data.timer.id, state->gc_id) == 0 )
				runGarbageCollector(state);
			else if( elem.data.timer.timer_type == MESSAGE_TIMEOUT )
				uponTimeout(state, &elem.data.timer);
			else {
				triggerRetransmissionContextEvent(state, &elem);
			}

			break;
		case YGG_MESSAGE:
			;
			MessageType type = popMessageType(&elem.data.msg);
			if(type == BROADCAST_MESSAGE)
				uponNewMessage(state, &elem.data.msg);
			else
				triggerRetransmissionContextEvent(state, &elem);
			break;
		case YGG_REQUEST:
			if( elem.data.request.proto_dest == BCAST_FRAMEWORK_PROTO_ID && elem.data.request.request == REQUEST ) {
				if( elem.data.request.request_type == FRAMEWORK_BCAST_MSG_REQ ) {
					uponBroadcastRequest(state, &elem.data.request);
				} else if ( elem.data.request.request_type == FRAMEWORK_STATS_REQ ) {
					uponStatsRequest(state, &elem.data.request);
				} else {
					triggerRetransmissionContextEvent(state, &elem);
				}
			}

			break;
		case YGG_EVENT:
			if( elem.data.event.proto_dest == BCAST_FRAMEWORK_PROTO_ID ) {
				// TODO:

				//else {
				triggerRetransmissionContextEvent(state, &elem);
				//}

			} else {
				char s[100];
				sprintf(s, "Received notification from protocol %d meant for protocol %d", elem.data.event.proto_origin, elem.data.event.proto_dest);
				ygg_log(BCAST_FRAMEWORK_PROTO_NAME, "NOTIFICATION", s);
			}

			break;
		default:
			; // On purpose
			char s[100];
			sprintf(s, "Got weird queue elem, type = %u", elem.type);
			ygg_log(BCAST_FRAMEWORK_PROTO_NAME, "MAIN LOOP", s);
			exit(-1);
		}

		// Release memory of elem payload
		free_elem_payload(&elem);
	}

	return NULL;
}

proto_def* framework_init(void* arg) {

	framework_state* state = malloc(sizeof(framework_state));
	state->args = (framework_args*) arg;

	proto_def* framework = create_protocol_definition(BCAST_FRAMEWORK_PROTO_ID, BCAST_FRAMEWORK_PROTO_NAME, state, NULL);
	proto_def_add_protocol_main_loop(framework, &framework_main_loop);

	// Initialize state variables
	init(state);

	triggerRetransmissionContextInit(state, framework);

	return framework;
}

static void init(framework_state* state) {
	initMessagesList(&state->seen_msgs);

	// Garbage Collector
	genUUID(state->gc_id);
	struct timespec _gc_interval;
	milli_to_timespec(&_gc_interval, state->args->gc_interval_s*1000);
	SetPeriodicTimer(&_gc_interval, state->gc_id, BCAST_FRAMEWORK_PROTO_ID, -1);

	bzero(&state->stats, sizeof(bcast_stats));

	getmyId(state->myID);

	clock_gettime(CLOCK_REALTIME, &state->current_time); // Update current time
}

short popMessageType(YggMessage* msg) {
	short type = 0;
	YggMessage_readPayload(msg, NULL, &type, sizeof(short));

	unsigned short dataLen = msg->dataLen - sizeof(short);
	char data[YGG_MESSAGE_PAYLOAD];

	memcpy(data, msg + sizeof(short), dataLen);

	memcpy(msg, data, dataLen);
	msg->dataLen = dataLen;

	return type;
}

void pushMessageType(YggMessage* msg, short type) {
	unsigned short dataLen = msg->dataLen + sizeof(short);
	char data[YGG_MESSAGE_PAYLOAD];

	memcpy(data, &type, sizeof(type));
	memcpy(data + sizeof(short), msg, msg->dataLen);

	memcpy(data, msg, dataLen);
	msg->dataLen = dataLen;
}

static void ComputeRetransmissionDelay(framework_state* state, message_item* msg_item) {

	unsigned long delay = triggerRetransmissionDelay(state, msg_item);

	msg_item->timer_duration = delay;

	struct timespec _delay;
	milli_to_timespec(&_delay, delay);

	if(timespec_is_zero(&_delay))
		_delay.tv_nsec = 1;

	//memcpy(&msg->timer_duration, &delay, sizeof(struct timespec));
	//timespec_add(&msg_item->timer_expiration_date, current_time, &delay);

	SetTimer(&_delay, msg_item->id, BCAST_FRAMEWORK_PROTO_ID, MESSAGE_TIMEOUT);
}

static void changePhase(framework_state* state, message_item* msg) {

	if( msg->current_phase < state->args->algorithm.n_phases ) {
		// Increment current current_phase
		msg->current_phase++;

		// Compute Current Phase's Retransmission Delay
		ComputeRetransmissionDelay(state, msg);
	} else {
		// Message is no longer active
		msg->active = false;
	}
}

static void DeliverMessage(framework_state* state, YggMessage* toDeliver) {
	deliver(toDeliver);
	state->stats.messages_delivered++;
}

static void _RetransmitMessage(framework_state* state, message_item* msg_item, bool keepCopy) {
	YggMessage m;
	serializeMessage(state, &m, msg_item);
	pushMessageType(&m, BROADCAST_MESSAGE);

	dispatch(&m);

	if(keepCopy) {
		// Remove Framework's header of the message
		bcast_header header;
		YggMessage toDeliver;
		void* context_header;
		deserializeMessage(&m, &header, &context_header, &toDeliver);

		addCopy(msg_item, &state->current_time, &header, context_header);
	}

	msg_item->retransmissions++;

	state->stats.messages_sent++;
}

static void RetransmitMessage(framework_state* state, message_item* msg) {
	_RetransmitMessage(state, msg, false);
}

static void uponBroadcastRequest(framework_state* state, YggRequest* req) {

	state->stats.messages_bcasted++;

	// Generate a unique message id
	uuid_t msg_id;
	genUUID(msg_id);

	// Insert on seen_msgs
	message_item* msg_item = newMessageItem(msg_id, req->proto_origin, req->payload, req->length, 1, &state->current_time, NULL, NULL, 0L);
	pushMessageItem(&state->seen_msgs, msg_item);

	// Retransmit Message and Keep a Copy
	_RetransmitMessage(state, msg_item, true);

	// Deliver the message to the upper layer
	DeliverMessage(state, &msg_item->toDeliver);

	// Check if there are more phases
	changePhase(state, msg_item);
}

static void uponNewMessage(framework_state* state, YggMessage* msg) {

	state->stats.messages_received++;

	// Remove Framework's header of the message
	bcast_header header;
	YggMessage toDeliver;
	void* context_header;
	deserializeMessage(msg, &header, &context_header, &toDeliver);

	// If New Msg
	if( !isInSeenMessages(&state->seen_msgs, header.msg_id) ){

		// Insert on seen_msgs
		message_item* msg_item = newMessageItem(header.msg_id, header.dest_proto, toDeliver.data, toDeliver.dataLen, 1, &state->current_time, &header, context_header, 0L);
		pushMessageItem(&state->seen_msgs, msg_item);

		// Deliver the message to the upper layer
		DeliverMessage(state, &msg_item->toDeliver);

		// Compute Current Phase's Retransmission Delay
		ComputeRetransmissionDelay(state, msg_item);

	} else {  // Duplicate message
		message_item* msg_item = getMessageItem(&state->seen_msgs, header.msg_id);
		addCopy(msg_item, &state->current_time, &header, context_header);

		// TODO: Alterar o timer?
	}
}

static void uponTimeout(framework_state* state, YggTimer* timer) {

	message_item* msg_item = getMessageItem(&state->seen_msgs, timer->id);

	assert(msg_item != NULL);
	if( msg_item != NULL ) {
		// Retransmit Message
		bool retransmit = triggerRetransmissionPolicy(state, msg_item);
		if(retransmit)
			RetransmitMessage(state, msg_item);
		else
			state->stats.messages_not_sent++;

		// Check if there are more phases
		changePhase(state, msg_item);

		/*if( compareTimespec(&current_time, &p_msg->expiration_date) >= 0 ) {
			popFromPending(&state->pending_msgs, p_msg );

			int retransmit = state->my_args->algorithm.retransmission->timeout(state->my_args->algorithm.retransmission->global_state, state->my_args->algorithm.retransmission->args, p_msg->phase, p_msg->retrasnmission_attr); // TODO: Alterar os parametros desta função
			onEndOfPhase(state, p_msg, &current_time, retransmit);
		} else {
			SetTimer(&p_msg->expiration_date, timer->id, BCAST_FRAMEWORK_PROTO_ID);
		}*/
	} else {
		ygg_log(BCAST_FRAMEWORK_PROTO_NAME, "TIMEOUT", "Received wierd timer!");
	}
}

static void serializeMessage(framework_state* state, YggMessage* m, message_item* msg_item) {

	assert(state != NULL);
	assert(m != NULL);
	assert(msg_item != NULL);

	// Initialize Broadcast Message Header
	bcast_header header;
	uuid_copy(header.msg_id, msg_item->id);
	getmyId(header.sender_id);
	header.dest_proto = msg_item->toDeliver.Proto_id;

	void* context_header = NULL;
	header.context_length = triggerRetransmissionContextHeader(state, msg_item, &context_header);

	int msg_size = BCAST_HEADER_LENGTH + header.context_length + msg_item->toDeliver.dataLen;
	assert( msg_size <= YGG_MESSAGE_PAYLOAD );

	YggMessage_initBcast(m, BCAST_FRAMEWORK_PROTO_ID);

	// Insert Header
	YggMessage_addPayload(m, (char*) &header, BCAST_HEADER_LENGTH);

	// Insert Context Header
	if( header.context_length > 0)
		YggMessage_addPayload(m, (char*) context_header, header.context_length);

	// Insert Payload
	YggMessage_addPayload(m, (char*) msg_item->toDeliver.data, msg_item->toDeliver.dataLen);

	// Free any allocated memory
	if(context_header != NULL )
		free(context_header);
}

static void deserializeMessage(YggMessage* m, bcast_header* header, void** context_header, YggMessage* toDeliver) {

	void* ptr = NULL;
	ptr = YggMessage_readPayload(m, ptr, header, BCAST_HEADER_LENGTH);

	if( header->context_length > 0 ) {
		*context_header = malloc(header->context_length);
		ptr = YggMessage_readPayload(m, ptr, *context_header, header->context_length);
	} else {
		*context_header = NULL;
	}

	unsigned short payload_size = m->dataLen - (BCAST_HEADER_LENGTH + header->context_length);

	YggMessage_initBcast(toDeliver, header->dest_proto);
	memcpy(toDeliver->srcAddr.data, m->srcAddr.data, WLAN_ADDR_LEN);
	toDeliver->dataLen = payload_size;
	ptr = YggMessage_readPayload(m, ptr, toDeliver->data, payload_size);
}

static void runGarbageCollector(framework_state* state) {
	struct timespec current_time, expiration;
	clock_gettime(CLOCK_REALTIME, &current_time);

	int counter = 0;

	double_list_item* current_item = state->seen_msgs.messages->head;
	while(current_item != NULL) {
		message_item* current_msg = (message_item*) current_item->data;

		if(!current_msg->active) {
			struct timespec* reception_time = &(((message_copy*)current_msg->copies->head->data)->reception_time);
			struct timespec _seen_expiration;
			milli_to_timespec(&_seen_expiration, state->args->seen_expiration_ms);
			add_timespec(&expiration, reception_time, &_seen_expiration);

			if( compare_timespec(&expiration, &current_time) <= 0 ) { // Message expired
				double_list_item* aux = current_item;
				current_item = current_item->next;

				message_item* aux_msg = double_list_remove_item(state->seen_msgs.messages, aux);

				deleteMessageItem(aux_msg);

				counter++;
			} else
				current_item = current_item->next;
		} else
			current_item = current_item->next;
	}

	//char s[100];
	//sprintf(s, " deleted %d messages. ", counter);
	//ygg_log(BCAST_FRAMEWORK_PROTO_NAME, "GC", s);
}

static void uponStatsRequest(framework_state* state, YggRequest* req) {
	unsigned short dest = req->proto_origin;
	YggRequest_init(req, BCAST_FRAMEWORK_PROTO_ID, dest, REPLY, FRAMEWORK_STATS_REQ);

	YggRequest_addPayload(req, (void*) &state->stats, sizeof(bcast_stats));

	deliverReply(req);

	YggRequest_freePayload(req);
}


