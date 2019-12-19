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

#include <stdlib.h>
#include <stdio.h>
//#include <stddef.h>
#include <uuid/uuid.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include "../Yggdrasil/Yggdrasil/core/proto_data_struct.h"
#include "data_structures/list.h"
#include "data_structures/hash_table.h"
#include "data_structures/ordered_list.h"
#include "utility/myUtils.h"
#include "../Yggdrasil/Yggdrasil/core/utils/hashfunctions.h"

#include "topology_discovery.h"

static short protoID = TOPOLOGY_DISCOVERY_PROTOCOL;
static char* protoName = "Topology Discovery";
static uuid_t process_id;

typedef struct _topology_discovery_state {
	double_list* neighbors;
	list* missed_announces;
	unsigned long version;
	bool announce_scheduled;
	bool missed_sheduled;

	unsigned int next_announce_ttl;

	struct timespec announce_delay;
	struct timespec missed_delay;
	struct timespec last_heartbeat;
	struct timespec last_gc_run;

	bool last_announce_empty;
	unsigned long last_announce_version;
	YggMessage last_announce;

	uuid_t announce_timer;
	uuid_t missed_timer;
	uuid_t heartbeat_timer;
	uuid_t gc_timer;

	struct timespec current_time;

	topology_discovery_args proto_args;
	topology_discovery_stats proto_stats;

	queue_t* dispatcher_queue;
} topology_discovery_state;

static void* topology_discovery_main_loop(main_loop_args* args);
static short topology_discovery_destructor(void*);

typedef struct _heartbeat_header {
	uuid_t process_id;
	unsigned long version;
} heartbeat_header;

typedef struct _neighbor {
	uuid_t id;
	unsigned long version;
	struct timespec last_heartbeat;
	YggMessage last_announce;
	unsigned int last_announce_hash;
} neighbor;

// Event Handlers
static void upon_init(topology_discovery_state* state);
static void upon_HeartbeatTimer(topology_discovery_state* state);
static void upon_AnnounceTimer(topology_discovery_state* state);
static void upon_MissedTimer(topology_discovery_state* state);
static void upon_GarbageCollectorTimer(topology_discovery_state* state);
static void upon_receiveAnnounce(topology_discovery_state* state, YggMessage* message, heartbeat_header* heartbeat);
static void upon_receiveMissedRequest(topology_discovery_state* state, YggMessage* message, heartbeat_header* heartbeat);
static void upon_receiveHeartbeat(topology_discovery_state* state, heartbeat_header* heartbeat);
static void upon_StatsRequest(topology_discovery_state* state, YggRequest* request);

static void serializeNewAnnounce(topology_discovery_state* state, YggMessage* msg, unsigned int ttl);
static void piggybackHeartbeat(topology_discovery_state* state, YggMessage* message, bool inc);
static int runGargbageCollector(topology_discovery_state* state);
static void sheduleNewAnnounce(topology_discovery_state* state, unsigned int ttl);
static void sheduleMissedAnnounce(topology_discovery_state* state);
static void notify_NewNeighbour(topology_discovery_state* state, neighbor* neigh);
static void notify_UpdateNeighbour(topology_discovery_state* state, neighbor* neigh);
static void notify_LostNeighbour(topology_discovery_state* state, neighbor* neigh);
static void notify_NewAnnounce(topology_discovery_state* state, YggMessage* msg);

static void printDic(hash_table* dictionary);

static bool equalID(uuid_t a, uuid_t b) {
	return uuid_compare(a, b) == 0;
}

static unsigned long uuid_hash(uuid_t id) {
	return RSHash(id, sizeof(uuid_t));
}

#pragma pack(1)
typedef struct _serialized_dic_entry {
	uuid_t id;
	unsigned char n_neighs;
} serialized_dic_entry;
#pragma pack()

typedef struct _my_dic_entry {
	list* l;
	unsigned int index;
} my_dic_entry;

typedef struct _missed_entry {
	uuid_t process_id;
	unsigned long last_seen_version;
} missed_entry;

static bool missed_cmp(missed_entry* e, uuid_t b) {
	return uuid_compare(e->process_id, b) == 0;
}

static bool neigh_cmp(neighbor* n, uuid_t b) {
	return uuid_compare(n->id, b) == 0;
}

typedef enum {
	ANNOUNCE = 0,
	MISSED = 1
} MsgType;

proto_def* topology_discovery_init(void* arg) {
	topology_discovery_state* state = malloc(sizeof(topology_discovery_state));
	state->proto_args = *((topology_discovery_args*) arg);

	getmyId(process_id);

	upon_init(state);

	state->dispatcher_queue = interceptProtocolQueue(PROTO_DISPATCH, protoID);

	proto_def* topology_discovery = create_protocol_definition(protoID, protoName, state, &topology_discovery_destructor);
	proto_def_add_protocol_main_loop(topology_discovery, &topology_discovery_main_loop);
	proto_def_add_produced_events(topology_discovery, (topology_discovery_events) DISCOVERY_EVENT_COUNT);

	return topology_discovery;
}

static short topology_discovery_destructor(void* args) {
	// TODO:
	return 0;
}

static void* topology_discovery_main_loop(main_loop_args* args) {
	topology_discovery_state* state = args->state;

	queue_t_elem elem;
	while (1) {
		queue_pop(args->inBox, &elem);

		// Update current time
		clock_gettime(CLOCK_MONOTONIC, &state->current_time);

		switch (elem.type) {
		case YGG_TIMER:
			if (elem.data.timer.proto_dest == protoID) {
				if (uuid_compare(elem.data.timer.id, state->heartbeat_timer) == 0) {
					upon_HeartbeatTimer(state);
				} else if (uuid_compare(elem.data.timer.id, state->gc_timer) == 0) {
					upon_GarbageCollectorTimer(state);
				} else if (uuid_compare(elem.data.timer.id, state->announce_timer) == 0) {
					upon_AnnounceTimer(state);
				} else if (uuid_compare(elem.data.timer.id, state->missed_timer) == 0) {
					upon_MissedTimer(state);
				}
			} else
				queue_push(state->dispatcher_queue, &elem);
			break;
		case YGG_MESSAGE:
			// UpStream message
			if (elem.data.msg.Proto_id == protoID) {
				heartbeat_header heartbeat;
				popPayload(&elem.data.msg, (char*) &heartbeat, sizeof(heartbeat));

				// Destination is this protocol
				if (elem.data.msg.Proto_id == protoID) {
					MsgType msg_type;
					popPayload(&elem.data.msg, (char*) &msg_type, sizeof(MsgType));

					if(msg_type == ANNOUNCE)
						upon_receiveAnnounce(state, &elem.data.msg, &heartbeat);
					else if(msg_type == MISSED)
						upon_receiveMissedRequest(state, &elem.data.msg, &heartbeat);

				} else { // Destination is other protocol
					upon_receiveHeartbeat(state, &heartbeat);
					filterAndDeliver(&elem.data.msg);
				}
			}
			// DownStream msg
			else {
				// Update our last announce timestamp
				piggybackHeartbeat(state, &elem.data.msg, true);

				// Insert into dispatcher queue
				queue_push(state->dispatcher_queue, &elem);
			}

			break;
		case YGG_REQUEST:
			if (elem.data.request.proto_dest == protoID) {
				if (elem.data.request.request == REQUEST) {
					if (elem.data.request.request_type == STATS_REQ) {
						upon_StatsRequest(state, &elem.data.request);
					}
				}
			} else
				queue_push(state->dispatcher_queue, &elem);
			break;
		case YGG_EVENT:
			if (elem.data.event.proto_dest != protoID)
				queue_push(state->dispatcher_queue, &elem);
			else {
				ygg_log(protoName, "ALERT", "Received unhandled event.");
			}
			break;
		default:
			; // On purpose
			char s[100];
			sprintf(s, "Got weird thing from queue, type = %u", elem.type);
			ygg_log(protoName, "ALERT", s);
		}

		// Release memory of elem payload
		free_elem_payload(&elem);
	}

	return NULL;
}

static void upon_init(topology_discovery_state* state) {

	state->neighbors = double_list_init();
	state->missed_announces = list_init();
	state->version = 1;

	state->next_announce_ttl = state->proto_args.horizon;

	state->announce_scheduled = true;
	state->missed_sheduled = true;

	bzero(&state->proto_stats, sizeof(topology_discovery_stats));

	state->last_announce_empty = true;
	state->last_announce_version = 0;

	// Update current time
	clock_gettime(CLOCK_MONOTONIC, &state->current_time);

	struct timespec t;

	// Schedule garbage collector
	uuid_generate_random(state->gc_timer);
	state->last_gc_run = state->current_time;
	milli_to_timespec(&t, state->proto_args.t_gc);
	if (timespec_is_zero(&t))
		t.tv_nsec = 1;
	SetTimer(&t, state->gc_timer, protoID, -1);

	// Schedule first announce
	uuid_generate_random(state->announce_timer);
	milli_to_timespec(&t, state->proto_args.max_jitter);
	random_timespec(&t, NULL, &t);
	if (timespec_is_zero(&t))
		t.tv_nsec = 1;
	SetTimer(&t, state->announce_timer, protoID, -1);
	state->announce_delay = t;

	// missed
	uuid_generate_random(state->missed_timer);
	state->missed_delay = zero_timespec;

	// Schedule heartbeat
	uuid_generate_random(state->heartbeat_timer);
	state->last_heartbeat = state->current_time;
	milli_to_timespec(&t, state->proto_args.heartbeat_period);
	if (timespec_is_zero(&t))
		t.tv_nsec = 1;
	SetTimer(&t, state->heartbeat_timer, protoID, -1);
	//add_timespec(&state->next_heartbeat, &state->current_time, &t);

	//assert(state->next_heartbeat.tv_sec >= 0 && state->next_heartbeat.tv_nsec >= 0 );
}

static void upon_HeartbeatTimer(topology_discovery_state* state) {

	// Compute the time for the next expected heartbeat
	unsigned long delta = 1000;
	struct timespec next_expected_heartbeat, t;
	milli_to_timespec(&next_expected_heartbeat, state->proto_args.heartbeat_period - delta);
	add_timespec(&next_expected_heartbeat, &next_expected_heartbeat, &state->last_heartbeat);

	bool heartbeat_too_long_ago = compare_timespec(&next_expected_heartbeat, &state->current_time) <= 0;

	// If the last heartbeat was a long time ago
	if ( heartbeat_too_long_ago ) {
		// Schedule new announce
		sheduleNewAnnounce(state, 1);

		milli_to_timespec(&t, state->proto_args.heartbeat_period /*+ state->proto_args.max_jitter*/);
		//add_timespec(&t, &t, &next_expected_heartbeat);
		//assert(compare_timespec(&t, &state->current_time) >= 0);
		//subtract_timespec(&t, &t, &state->current_time);

		/*if (timespec_is_zero(&t))
			t.tv_nsec = 1;*/

		SetTimer(&t, state->heartbeat_timer, protoID, -1);
	}

	// Re-schedule heartbeat timer
	else {
		//subtract_timespec(&t, &next_expected_heartbeat, &state->current_time);
		/*milli_to_timespec(&t, state->proto_args.heartbeat_period);
		add_timespec(&t, &t, &state->last_heartbeat);
		subtract_timespec(&t, &state->current_time, &t);*/

		subtract_timespec(&t, &next_expected_heartbeat, &state->current_time);

		/*if (timespec_is_zero(&t))
			t.tv_nsec = 1;*/

		SetTimer(&t, state->heartbeat_timer, protoID, -1);
	}
}

static void upon_AnnounceTimer(topology_discovery_state* state) {
	runGargbageCollector(state);

	YggMessage msg;
	if(state->last_announce_empty || state->last_announce_version < state->version) {
		state->last_announce_empty = false;

		// Create a new Announce Message
		serializeNewAnnounce(state, &state->last_announce, state->next_announce_ttl);
		state->next_announce_ttl = 0;
		state->last_announce_version = state->version;

		if(state->version > 1)
			notify_NewAnnounce(state, &state->last_announce);
	}
	msg = state->last_announce;

	// DEBUG
	char* str;
	printAnnounce(state->last_announce.data, state->last_announce.dataLen, state->proto_args.horizon, &str);
	printf("%s\n%s", "Serialized Announce", str);
	free(str);

	MsgType msg_type = ANNOUNCE;
	pushPayload(&msg, (char*) &msg_type, sizeof(MsgType), protoID, getBroadcastAddr());

	// Insert heartbeat information
	piggybackHeartbeat(state, &msg, false);

	// Insert into dispatcher queue
	queue_t_elem elem = {
			.data.msg = msg,
			.type = YGG_MESSAGE
	};
	queue_push(state->dispatcher_queue, &elem);

	//ygg_log(protoName, "NEW ANNOUNCE", "New announce sent!");
	state->proto_stats.announces++;

	state->announce_scheduled = false;
}

static void upon_MissedTimer(topology_discovery_state* state) {
	YggMessage msg;

	YggMessage_initBcast(&msg, protoID);

	YggMessage_addPayload(&msg, (char*) &state->missed_announces->size, sizeof(short));

	missed_entry* e = NULL;
	while( (e = list_remove_head(state->missed_announces)) ) {
		YggMessage_addPayload(&msg, (char*) e, sizeof(missed_entry));

		free(e);
	}

	MsgType msg_type = MISSED;
	pushPayload(&msg, (char*) &msg_type, sizeof(MsgType), protoID, getBroadcastAddr());

	// Insert heartbeat information
	piggybackHeartbeat(state, &msg, false);

	// Insert into dispatcher queue
	queue_t_elem elem;
	elem.data.msg = msg;
	elem.type = YGG_MESSAGE;
	queue_push(state->dispatcher_queue, &elem);

	//ygg_log(protoName, "NEW ANNOUNCE", "New announce sent!");
	state->proto_stats.missed_requests++;

	state->missed_sheduled = false;
}


static void upon_receiveMissedRequest(topology_discovery_state* state, YggMessage* message, heartbeat_header* heartbeat) {

	upon_receiveHeartbeat(state, heartbeat);

	void* ptr = NULL;

	int size = 0;
	YggMessage_readPayload(message, ptr, &size, sizeof(short));

	missed_entry e;
	for(int i = 0; i < size; i++) {
		YggMessage_readPayload(message, ptr, &e, sizeof(e));

		if(uuid_compare(e.process_id, process_id) == 0) {
			sheduleNewAnnounce(state, state->proto_args.horizon);
		}
	}
}

static void upon_receiveAnnounce(topology_discovery_state* state, YggMessage* message, heartbeat_header* heartbeat) {

	// DEBUG
	char* str;
	printAnnounce(message->data, message->dataLen, state->proto_args.horizon, &str);
	printf("%s\n%s", "Received Announce", str);
	free(str);

	// If in missed, remove
	missed_entry* e = list_find_item(state->missed_announces, (comparator_function) &missed_cmp, heartbeat->process_id);
	if(e != NULL){
		if(e->last_seen_version < heartbeat->version) {
			list_remove_item(state->missed_announces, (comparator_function) &missed_cmp, heartbeat->process_id);
		}
	}

	neighbor* neigh = (neighbor*) double_list_find(state->neighbors, (comparator_function) &equalID, heartbeat->process_id);
	if(neigh != NULL) {
		neigh->last_heartbeat = state->current_time;

		printf("old neigh version: %lu new neigh version: %lu\n", neigh->version , heartbeat->version);

		if(neigh->version < heartbeat->version) {
			neigh->version = heartbeat->version;
			neigh->last_announce = *message;

			unsigned int ttl = 0;
			YggMessage_readPayload(message, NULL, &ttl, sizeof(unsigned char));

			// TODO: TTL é inutil! Mas não posso remover para as camadas de cima conseguirem processar a msg (precisam que o primeiro byte seja o horizonte)

			/*if( ttl > 1 )
				sheduleNewAnnounce(state, ttl-1);

			state->version++;

			notify_UpdateNeighbour(state, neigh);*/

			if(ttl > 0) {
				unsigned int new_hash = RSHash(message->data, message->dataLen);

				if(new_hash != neigh->last_announce_hash) {
					neigh->last_announce_hash = new_hash;
					state->version++;
					sheduleNewAnnounce(state, ttl-1);
					notify_UpdateNeighbour(state, neigh);
				}
			}
		}
	} else {
		neigh = malloc(sizeof(neighbor)); // TODO: Criar funções para isto
		uuid_copy(neigh->id, heartbeat->process_id);
		neigh->last_heartbeat = state->current_time;
		neigh->version = heartbeat->version;
		neigh->last_announce = *message;

		double_list_add_item_to_tail(state->neighbors, neigh);

		state->version++;

		//current_hash = new_hash

		notify_NewNeighbour(state, neigh);

		state->proto_stats.new_neighbours++;

		sheduleNewAnnounce(state, state->proto_args.horizon);
	}
}

static void issue_missed_request(topology_discovery_state* state, heartbeat_header* heartbeat) {
	state->proto_stats.misses++;

	// If in missed, remove
	missed_entry* e = (missed_entry*) list_find_item(state->missed_announces, (comparator_function) &missed_cmp, heartbeat->process_id);
	if(e != NULL){
		if(e->last_seen_version < heartbeat->version)
			e->last_seen_version = heartbeat->version;
	} else {
		e = malloc( sizeof(missed_entry) );
		uuid_copy(e->process_id, heartbeat->process_id);
		e->last_seen_version = heartbeat->version;
		list_add_item_to_tail(state->missed_announces, e);
	}

	sheduleMissedAnnounce(state);
}

static void upon_receiveHeartbeat(topology_discovery_state* state, heartbeat_header* heartbeat) {

	neighbor* neigh = (neighbor*) double_list_find(state->neighbors, (comparator_function) &neigh_cmp, heartbeat->process_id);
	if(neigh != NULL) {
		neigh->last_heartbeat = state->current_time;

		if(neigh->version < heartbeat->version) {
			issue_missed_request(state, heartbeat);
		}
	} else {
		issue_missed_request(state, heartbeat);
	}
}

static void upon_GarbageCollectorTimer(topology_discovery_state* state) {
	struct timespec next_expected_gc_run;
	milli_to_timespec(&next_expected_gc_run, state->proto_args.t_gc);
	add_timespec(&next_expected_gc_run, &state->last_gc_run, &next_expected_gc_run);

	if (compare_timespec(&next_expected_gc_run, &state->current_time) <= 0) {
		int deleted = runGargbageCollector(state);
		if (deleted > 0) {
			sheduleNewAnnounce(state, state->proto_args.horizon);
		}
	} else {
		subtract_timespec(&next_expected_gc_run, &next_expected_gc_run, &state->current_time);
		SetTimer(&next_expected_gc_run, state->gc_timer, protoID, -1);
	}
}

static void piggybackHeartbeat(topology_discovery_state* state, YggMessage* message, bool inc) {
	// Update last announce timestamp
	state->last_heartbeat = state->current_time;

	heartbeat_header heartbeat;
	heartbeat.version = state->version;
	getmyId(heartbeat.process_id);
	pushPayload(message, (char*) &heartbeat, sizeof(heartbeat), protoID, getBroadcastAddr());

	if(inc)
		state->proto_stats.piggybacked_heartbeats++;
}

static int runGargbageCollector(topology_discovery_state* state) {
	int deleted = 0;

	struct timespec aux_t;

	double_list_item* current_item = state->neighbors->head;
	while(current_item != NULL) {
		neighbor* current_neigh = (neighbor*) current_item->data;

		unsigned long max_delay = state->proto_args.misses*(state->proto_args.heartbeat_period+state->proto_args.max_jitter);
		milli_to_timespec(&aux_t, max_delay);
		add_timespec(&aux_t, &aux_t, &current_neigh->last_heartbeat);
		if( compare_timespec(&aux_t, &state->current_time) <= 0 ) {
			// REMOVE
			double_list_item* to_delete  = current_item;
			current_item = current_item->next;

			neighbor* current_neigh = (neighbor*) double_list_remove_item(state->neighbors, to_delete);

			notify_LostNeighbour(state, current_neigh);

			free(current_neigh);

			deleted++;
		} else
			current_item = current_item->next;
	}

	state->last_gc_run = state->current_time;

	state->proto_stats.lost_neighbours += deleted;

	if(deleted > 0)
		state->version++;

	return deleted;
}

static void sheduleNewAnnounce(topology_discovery_state* state, unsigned int ttl) {

	state->next_announce_ttl = iMax(state->next_announce_ttl, ttl);

	struct timespec t;
	random_mili_to_timespec(&t, state->proto_args.max_jitter); // TODO: não deveria utilizar o critério dos vizinhos?

	// Set new timer
	if(!state->announce_scheduled) {
		state->announce_scheduled = true;

		state->announce_delay = t;

		SetTimer(&t, state->announce_timer, protoID, -1);
	}

	// Timer already set
	else {
		// If the current timer is too far away and the new delay is less than the current delay
		struct timespec max_jitter;
		milli_to_timespec(&max_jitter, state->proto_args.max_jitter);
		if( compare_timespec(&state->announce_delay, &max_jitter) > 0 && compare_timespec(&t, &state->announce_delay) < 0 ) {
			state->announce_delay = t;

			CancelTimer(state->announce_timer, protoID);
			SetTimer(&t, state->announce_timer, protoID, -1);
		}
	}
}

static void sheduleMissedAnnounce(topology_discovery_state* state) {

	struct timespec t;
	random_mili_to_timespec(&t, state->proto_args.max_jitter);

	// Set new timer
	if(!state->missed_sheduled) {
		state->missed_sheduled = true;

		state->missed_delay = t;

		SetTimer(&t, state->missed_timer, protoID, -1);
	}

	// Timer already set
	else {
		// If the current timer is too far away and the new delay is less than the current delay
		struct timespec max_jitter;
		milli_to_timespec(&max_jitter, state->proto_args.max_jitter);
		if( compare_timespec(&state->missed_delay, &max_jitter) > 0 && compare_timespec(&t, &state->missed_delay) < 0 ) {
			state->missed_delay = t;

			CancelTimer(state->missed_timer, protoID);
			SetTimer(&t, state->missed_timer, protoID, -1);
		}
	}
}

//static void free_hash_table_item(hash_table_item* current) {
//	if(current->key != NULL)
//		free(current->key);
//	if(current->value != NULL) {
//		my_dic_entry* e = current->value;
//		free(e->l);
//		free(current->value);
//	}
//}

//static void serializeNewAnnounce(topology_discovery_state* state, YggMessage* msg, unsigned int ttl) {
//
//	assert(state->neighbors->size <= 255); // To not overflow
//
//	unsigned int horizon = state->proto_args.horizon;
//
//	YggMessage_initBcast(msg, protoID);
//
//	unsigned char ptrs[horizon+1]; bzero(ptrs, sizeof(ptrs));
//
//	ptrs[0] = 1;
//
//	hash_table* dictionary = hash_table_init((hashing_function) &uuid_hash, (comparator_function) &equalID);
//	list** levels = malloc((horizon+1)*sizeof(list*));
//	for(int i = 0; i <= horizon; i++) {
//		levels[i] = list_init();
//	}
//
//	unsigned int current_index = 0;
//
//	{ // Insert myself
//		my_dic_entry* e = malloc(sizeof(my_dic_entry));
//		e->l = list_init();
//		e->index = current_index++;
//		unsigned char* x = malloc(sizeof(uuid_t));
//		uuid_copy(x, process_id);
//		hash_table_insert(dictionary, x, e);
//
//		/*unsigned char* x2 = malloc(sizeof(uuid_t));
//		uuid_copy(x2, x);
//		list_add_item_to_tail(levels[0], x2);*/
//
//		list_add_item_to_tail(levels[0], x);
//	}
//
//	for(int h = 1; h <= horizon; h++) {
//		unsigned int neigh_level = h-1;
//
//		double_list_item* current_item = state->neighbors->head;
//		while(current_item != NULL) {
//			neighbor* current_neigh = (neighbor*) current_item->data;
//
//			void* announce_ptr = current_neigh->last_announce.data + sizeof(unsigned char);
//
//			// Read Pointers
//			unsigned char neigh_ptrs[horizon+1];
//			memcpy(neigh_ptrs, announce_ptr, sizeof(neigh_ptrs));
//			announce_ptr += sizeof(neigh_ptrs);
//
//			unsigned int neigh_dic_size = neigh_ptrs[horizon];
//
//			// Read Dictionary
//			serialized_dic_entry neigh_dic[neigh_dic_size];
//			memcpy(neigh_dic, announce_ptr, neigh_dic_size*sizeof(serialized_dic_entry));
//			announce_ptr += neigh_dic_size*sizeof(serialized_dic_entry);
//
//			unsigned int level_start = neigh_level == 0 ? 0 : neigh_ptrs[neigh_level-1];
//			unsigned int level_size = neigh_level == 0 ? 1 : neigh_ptrs[neigh_level] - level_start;
//			for(int i = level_start; i < level_size; i++) {
//				// Get neighbors list and add to dictionary
//				unsigned char neighs_neighs[neigh_dic[i].n_neighs];
//
//				unsigned char sum = 0;
//				for(int j = 0; j < i; j++) {
//					sum += neigh_dic[j].n_neighs;
//				}
//				void* start = announce_ptr + sum*sizeof(unsigned char);
//
//				memcpy(neighs_neighs, start, neigh_dic[i].n_neighs);
//
//				//list* l = hash_table_find_value(dictionary, neigh_dic[i].id);
//				my_dic_entry* e = hash_table_find_value(dictionary, neigh_dic[i].id);
//				if(e == NULL) {
//					e = malloc(sizeof(my_dic_entry));
//					e->l = list_init();
//					e->index = current_index++;
//					unsigned char* x = malloc(sizeof(uuid_t));
//					uuid_copy(x, neigh_dic[i].id);
//					hash_table_insert(dictionary, x, e);
//
//					/*unsigned char* x2 = malloc(sizeof(uuid_t));
//					uuid_copy(x2, x);
//					list_add_item_to_tail(levels[h-1], x2);*/
//					list_add_item_to_tail(levels[h-1], x);
//				}
//				for(int j = 0; j <  neigh_dic[i].n_neighs; j++) {
//					unsigned char* id = neigh_dic[neighs_neighs[j]].id;
//					if(list_find_item(e->l, (comparator_function) &equalID, id) == NULL) {
//						/*unsigned char* x = malloc(sizeof(uuid_t));
//						uuid_copy(x, id);
//						list_add_item_to_tail(e->l, x);*/
//						list_add_item_to_tail(e->l, id);
//					}
//				}
//			}
//			current_item = current_item->next;
//		}
//
//		//ptrs[h-1] = levels[h-1]->size + (h==1 ? 0 : ptrs[h-2]);
//		//ptrs[h] = levels[h]->size + (h==0 ? 0 : ptrs[h-1]);
//		ptrs[h] = levels[h]->size + ptrs[h-1];
//	}
//
//	YggMessage aux;
//
//	YggMessage_addPayload(msg, (char*) &ttl, sizeof(unsigned char));
//	YggMessage_addPayload(msg, (char*) ptrs, sizeof(ptrs));
//
//	// Serialize Dic
//	for(int h = 0; h <= horizon; h++) {
//		unsigned char* current = NULL;
//		while((current = list_remove_head(levels[h]))) {
//			//list* l = hash_table_find_value(dictionary, current);
//			my_dic_entry* e = hash_table_find_value(dictionary, current);
//			assert( e != NULL );
//
//			YggMessage_addPayload(msg, (char*) current, sizeof(uuid_t));
//			YggMessage_addPayload(msg, (char*) &e->l->size, sizeof(unsigned char));
//
//			//if( h < horizon) {
//			unsigned char* x = NULL;
//			while((x = list_remove_head(e->l))) {
//				my_dic_entry* e2 = hash_table_find_value(dictionary, x);
//				assert(e2 != NULL);
//
//				if( h < horizon )
//					YggMessage_addPayload(&aux, (char*) &(e2->index), sizeof(e2->index));
//
//				//free(x);
//			}
//			//}
//			//free(l);
//			//free(current);
//		}
//		free(levels[h]);
//	}
//
//	YggMessage_addPayload(msg, (char*) aux.data, aux.dataLen);
//
//	//hash_table_delete_custom(dictionary, NULL);
//	hash_table_delete_custom(dictionary, &free_hash_table_item);
//}

struct announce_state {
	hash_table* dictionary;
	ordered_list** levels;
};

static void process_node(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id, unsigned char n_neighs) {
	struct announce_state* _state = (struct announce_state*)state;
	hash_table* dictionary = _state->dictionary;
	ordered_list** levels = _state->levels;

	//	printf("2.x.1.%d.1\n", index);
	//	printDic(dictionary);

	if(!is_last_level) {
		my_dic_entry* e = hash_table_find_value(dictionary, id);
		if(e == NULL) {
			unsigned char* x = malloc(sizeof(uuid_t));
			uuid_copy(x, id);
			hash_table_insert(dictionary, x, list_init());

			//if(!is_last_level)
			ordered_list_add_item(levels[current_level+1], x);
			//list_add_item_to_tail(levels[current_level+1], x);
		}
	}

	//	printf("2.x.1.%d.2\n", index);
	//	printDic(dictionary);
}

static void process_edge(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id1, unsigned char* id2) {
	struct announce_state* _state = (struct announce_state*)state;
	hash_table* dictionary = _state->dictionary;

	//	printf("2.x.2.%d.1\n", index);
	//	printDic(dictionary);

	//	if(is_last_level) {
	//		list* l2 = hash_table_find_value(dictionary, id2);
	//		if(l2 != NULL) { // id2 is form h-1 level
	//			if(list_find_item(l, (comparator_function) &equalID, id2) == NULL) {
	//				unsigned char* x = malloc(sizeof(uuid_t));
	//				uuid_copy(x, id2);
	//				list_add_item_to_tail(l, x);
	//			}
	//		}
	//	} else {
	//		if(list_find_item(l, (comparator_function) &equalID, id2) == NULL) {
	//			unsigned char* x = malloc(sizeof(uuid_t));
	//			uuid_copy(x, id2);
	//			list_add_item_to_tail(l, x);
	//		}
	//	}

	if(!is_last_level) {
		list* l = hash_table_find_value(dictionary, id1);
		assert(l != NULL);

		if( hash_table_find_value(dictionary, id2) != NULL) { // if node id2 is within horizon
			if(list_find_item(l, (comparator_function) &equalID, id2) == NULL) {
				unsigned char* x = malloc(sizeof(uuid_t));
				uuid_copy(x, id2);
				list_add_item_to_tail(l, x);
			}
		}
	}

	//	printf("2.x.2.%d.2\n", index);
	//	printDic(dictionary);
}

static int getIndex(serialized_dic_entry* serialized_dic, unsigned int size, unsigned char* id) {
	for(int i = 0; i < size; i++) {
		if(uuid_compare(id, serialized_dic[i].id) == 0)
			return i;
	}
	return -1;
}

// DEBUG
static void printDic(hash_table* dictionary) {
	for(int i = 0; i < dictionary->array_size; i++) {
		for(double_list_item* current = dictionary->array[i]->head; current; current = current->next) {
			hash_table_item* it = (hash_table_item*)current->data;

			char buffer[1000];

			char* ptr = buffer;

			char id_str[UUID_STR_LEN];
			uuid_unparse(it->key, id_str);
			sprintf(ptr, "%s : { ", id_str);
			ptr += strlen(ptr);

			for(list_item* current = ((list*)it->value)->head; current; current = current->next) {
				uuid_unparse((unsigned char*)current->data, id_str);
				sprintf(ptr, "%s ", id_str);
				ptr += strlen(ptr);
			}

			sprintf(ptr, "} ");
			ptr += strlen(ptr);

			printf("%s\n", buffer);
		}
	}
}

static void serializeNewAnnounce(topology_discovery_state* state, YggMessage* msg, unsigned int ttl) {
	assert(state->neighbors->size <= 255); // To not overflow

	unsigned int horizon = state->proto_args.horizon;
	YggMessage_initBcast(msg, protoID);

	unsigned char ptrs[horizon+1]; bzero(ptrs, sizeof(ptrs));

	hash_table* dictionary = hash_table_init((hashing_function) &uuid_hash, (comparator_function) &equalID);
	ordered_list** levels = malloc((horizon+1)*sizeof(ordered_list*));
	for(int i = 0; i <= horizon; i++) {
		levels[i] = ordered_list_init((comp_function)&uuid_compare);
	}

	//	printf("1\n");
	//	printDic(dictionary);

	// Insert myself
	list* myNeighs = list_init();
	unsigned char* x = malloc(sizeof(uuid_t));
	uuid_copy(x, process_id); // TODO: transformar o process_id em parte do estado
	hash_table_insert(dictionary, x, myNeighs);
	//list_add_item_to_tail(levels[0], x);
	ordered_list_add_item(levels[0], x);

	printf("2\n");
	printDic(dictionary);

	//	int i = 0;
	for(double_list_item* current_item = state->neighbors->head; current_item; current_item = current_item->next) {
		neighbor* current_neigh = (neighbor*) current_item->data;

		struct announce_state _state = {
				.dictionary = dictionary,
				.levels = levels,
		};
		processAnnounce(&_state, current_neigh->last_announce.data, current_neigh->last_announce.dataLen, horizon, &process_node, &process_edge);

		if(list_find_item(myNeighs, (comparator_function) &equalID, current_neigh->id) == NULL) {
			unsigned char* x = malloc(sizeof(uuid_t));
			uuid_copy(x, current_neigh->id);
			list_add_item_to_tail(myNeighs, x);
		}

		//		printf("2.%d\n", i++);
		//		printDic(dictionary);
	}

	printf("3\n");
	printDic(dictionary);

	for(int h = 0; h <= horizon; h++) {
		ptrs[h] = levels[h]->size + ptrs[h-1];
	}

	// Start serializing
	YggMessage_addPayload(msg, (char*) &ttl, sizeof(unsigned char));
	YggMessage_addPayload(msg, (char*) ptrs, sizeof(ptrs));

	// Serialize Dictionary
	serialized_dic_entry serialized_dic[dictionary->n_items];
	int current_index = 0;

	for(int h = 0; h <= horizon; h++) {
		unsigned char* current = NULL;
		while((current = ordered_list_remove_head(levels[h]))) {
			list* current_neighs = hash_table_find_value(dictionary, current);
			assert( current_neighs != NULL );

			uuid_copy(serialized_dic[current_index].id, current);
			serialized_dic[current_index].n_neighs = current_neighs->size;

			current_index++;
		}
		free(levels[h]);
	}
	free(levels);

	printf("4\n");
	printDic(dictionary);

	YggMessage_addPayload(msg, (char*)serialized_dic, sizeof(serialized_dic));

	int processed = 0;
	for(int h = 0; h <= horizon; h++) {
		int level_size = ptrs[h];
		int level_start = h == 0 ? 0 : ptrs[h-1];
		for(int node = level_start; node < level_size; node++) {
			//bool is_last_level = (h == horizon);

			processed++;

			list* current_neighs = hash_table_find_value(dictionary, serialized_dic[node].id);
			assert( current_neighs != NULL );

			/*unsigned char* x = NULL;
		while((x = list_remove_head(current_neighs))) {
			int index = getIndex(serialized_dic, dictionary->n_items, x);
			assert(index >= 0);

			free(x);

			unsigned char _index = (unsigned char)index;

			if( current_level < horizon )
				YggMessage_addPayload(msg, (char*)&_index, sizeof(unsigned char));
		}*/

			unsigned char* x = NULL;
//			if( h <= horizon ) {
				int size = current_neighs->size;
				int neighs_array[size];
				for(int i = 0; i < size; i++) {
					x = list_remove_head(current_neighs);
					neighs_array[i] = getIndex(serialized_dic, dictionary->n_items, x);
					assert(neighs_array[i] >= 0);
					free(x);
				}
				insertionSort(neighs_array, sizeof(int), size, (comp_function)&compare_int);
				for(int i = 0; i < size; i++) {
					unsigned char n = (unsigned char) neighs_array[i];
					YggMessage_addPayload(msg, (char*)&n, sizeof(unsigned char));
				}
//			}
//			else {
//				while((x = list_remove_head(current_neighs))) {
//					free(x);
//				}
//			}
		}
	}
	assert(processed == ptrs[horizon]);

	hash_table_delete(dictionary);
}

void processAnnounce(void* state, void* announce, int size, int horizon, void (*process_node)(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id, unsigned char n_neighs), void (*process_edge)(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id1, unsigned char* id2)) {
	if(horizon < 0)
		memcpy(&horizon, announce, sizeof(unsigned char));

	void* ptr = announce + sizeof(unsigned char);

	// Read Pointers
	unsigned char ptrs[horizon+1];
	memcpy(ptrs, ptr, sizeof(ptrs));
	ptr += sizeof(ptrs);

	unsigned int dic_size = ptrs[horizon];

	// Read Dictionary
	serialized_dic_entry dictionary[dic_size];
	memcpy(dictionary, ptr, dic_size * sizeof(serialized_dic_entry));
	ptr += dic_size * sizeof(serialized_dic_entry);

	// Process every node first
	if(process_node != NULL) {
		int processed = 0;
		for(int h = 0; h <= horizon; h++) {
			int level_size = ptrs[h];
			int level_start = h == 0 ? 0 : ptrs[h-1];
			for(int node = level_start; node < level_size; node++) {
				bool is_last_level = (h == horizon);

				processed++;

				// Process Node
				process_node(state, h, is_last_level, node, dictionary[node].id, dictionary[node].n_neighs);
			}
		}
		assert(processed == dic_size);
	}


	// Process each edge
	if(process_node != NULL) {
		int processed = 0;
		for(int h = 0; h <= horizon; h++) {
			int level_size = ptrs[h];
			int level_start = h == 0 ? 0 : ptrs[h-1];
			for(int node = level_start; node < level_size; node++) {
				bool is_last_level = (h == horizon);

				processed++;

				// get list of indexes of the neighbors
				unsigned int n_neighbors = dictionary[node].n_neighs;

				if(n_neighbors > 0) {
					unsigned char neighbors[n_neighbors];
					//void* start = ptr + sum*sizeof(unsigned char);
					memcpy(neighbors, ptr, n_neighbors);
					ptr += n_neighbors*sizeof(unsigned char);

					for(int neigh = 0; neigh < n_neighbors; neigh++) {
						// Process Edge
						process_edge(state, h, is_last_level, node, dictionary[node].id, dictionary[neighbors[neigh]].id);
					}
				}
			}
		}
		assert(processed == dic_size);

		//		int current_level = 0, counter = ptrs[current_level];
		//				int sum = 0;
		//		for(int node = 0; node < dic_size; node++) {
		//			if(node >= counter) {
		//				current_level++;
		//				counter = ptrs[current_level];
		//			}

		//		bool is_last_level = current_level == horizon;
		//
		//		// get list of indexes of the neighbors
		//		unsigned int n_neighbors = dictionary[node].n_neighs;
		//
		//		if(n_neighbors > 0) {
		//			unsigned char neighbors[n_neighbors];
		//			void* start = ptr + sum*sizeof(unsigned char);
		//			memcpy(neighbors, start, n_neighbors);
		//
		//			for(int neigh = 0; neigh < n_neighbors; neigh++) {
		//				// Process Edge
		//				process_edge(state, current_level, is_last_level, node, dictionary[node].id, dictionary[neighbors[neigh]].id);
		//			}
		//		}
		//
		//		sum += n_neighbors;
	}
}

struct print_announce_state {
	char* lines;
	unsigned int* lens;
	unsigned int buff_size;
};

static void process_node_print(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id, unsigned char n_neighs) {
	char* lines = ((struct print_announce_state*)state)->lines;
	unsigned int* lens =  ((struct print_announce_state*)state)->lens;
	unsigned int buff_size = ((struct print_announce_state*)state)->buff_size;

	char* ptr = (lines + (index*buff_size)) + lens[index];

	char id_str[UUID_STR_LEN];
	uuid_unparse(id, id_str);

	sprintf(ptr, "%s n=%d h=%d \n", id_str, n_neighs, current_level);
	int len = strlen(ptr);

	assert(lens[index] + len < buff_size);

	lens[index] += len;
}

static void process_edge_print(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id1, unsigned char* id2) {
	char* lines = ((struct print_announce_state*)state)->lines;
	unsigned int* lens =  ((struct print_announce_state*)state)->lens;
	unsigned int buff_size = ((struct print_announce_state*)state)->buff_size;

	char* ptr = (lines + (index*buff_size)) + lens[index];

	char id_str[UUID_STR_LEN];
	uuid_unparse(id2, id_str);

	sprintf(ptr, "\t %s \n", id_str);
	int len = strlen(ptr);

	assert(lens[index] + len < buff_size);

	lens[index] += len;
}

void printAnnounce(void* announce, int size, int horizon, char** str) {

	unsigned int line_size = UUID_STR_LEN + 2*3 + 11;

	unsigned int dic_size = (unsigned int)*((unsigned char*)(announce + (horizon+1)));
	serialized_dic_entry* dic = announce + (horizon+1) + 1;

	unsigned int max_neighs = 0;
	for(int i = 0; i < dic_size; i++) {
		max_neighs = iMax(max_neighs, dic[i].n_neighs);
	}

	unsigned int buff_size = line_size*(max_neighs+1);

	char lines[dic_size][buff_size];
	bzero(lines, sizeof(lines));
	unsigned int lens[dic_size]; bzero(lens, dic_size*sizeof(unsigned int));

	struct print_announce_state _state = {
			.lines = (char*)lines,
			.lens = lens,
			.buff_size = buff_size
	};
	processAnnounce(&_state, announce, size, horizon, &process_node_print, &process_edge_print);

	unsigned int total_len = 0;
	for(int i = 0; i < dic_size; i++)
		total_len += lens[i];
	total_len++;

	*str = calloc(total_len, sizeof(char));
	char* ptr = *str;
	for(int i = 0; i < dic_size; i++) {
		sprintf(ptr, "%s", lines[i]);
		ptr += lens[i];
	}
}

static void upon_StatsRequest(topology_discovery_state* state, YggRequest* request) {
	YggRequest reply;
	YggRequest_init(&reply, protoID, request->proto_origin, REPLY, STATS_REQ);
	YggRequest_addPayload(&reply, (void*) &state->proto_stats, sizeof(topology_discovery_stats));
	deliverReply(&reply);
	YggRequest_freePayload(&reply);
}

static void notify_NewNeighbour(topology_discovery_state* state, neighbor* neigh) {
	YggEvent e;
	YggEvent_init(&e, protoID, NEIGHBOR_FOUND);
	YggEvent_addPayload(&e, neigh->last_announce.data, neigh->last_announce.dataLen);
	memcpy(e.payload, &(state->proto_args.horizon), sizeof(unsigned char));
	deliverEvent(&e);
	YggEvent_freePayload(&e);
}

static void notify_UpdateNeighbour(topology_discovery_state* state, neighbor* neigh) {
	YggEvent e;
	YggEvent_init(&e, protoID, NEIGHBOR_UPDATE);
	YggEvent_addPayload(&e, neigh->last_announce.data, neigh->last_announce.dataLen);
	memcpy(e.payload, &(state->proto_args.horizon), sizeof(unsigned char));
	deliverEvent(&e);
	YggEvent_freePayload(&e);
}

static void notify_LostNeighbour(topology_discovery_state* state, neighbor* neigh) {
	assert(neigh->last_announce.dataLen > 0);

	YggEvent e;
	YggEvent_init(&e, protoID, NEIGHBOR_LOST);
	YggEvent_addPayload(&e, neigh->last_announce.data, neigh->last_announce.dataLen);
	memcpy(e.payload, &(state->proto_args.horizon), sizeof(unsigned char));
	deliverEvent(&e);
	YggEvent_freePayload(&e);
}

static void notify_NewAnnounce(topology_discovery_state* state, YggMessage* msg) {
	YggEvent e;
	YggEvent_init(&e, protoID, NEIGHBORHOOD_UPDATE);
	YggEvent_addPayload(&e, msg->data, msg->dataLen);
	memcpy(e.payload, &(state->proto_args.horizon), sizeof(unsigned char));
	deliverEvent(&e);
	YggEvent_freePayload(&e);
}

void new_topology_discovery_args(topology_discovery_args* d_args, unsigned int horizon, unsigned long max_jitter, unsigned long heartbeat_period, unsigned long t_gc, unsigned int max_heartbeats_missed_to_consider_failed ) {
	d_args->horizon = horizon;
	d_args->max_jitter = max_jitter;
	d_args->heartbeat_period = heartbeat_period;
	d_args->t_gc = t_gc;
	d_args->misses = max_heartbeats_missed_to_consider_failed;
}


