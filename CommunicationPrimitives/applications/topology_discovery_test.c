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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include "Yggdrasil_lowlvl.h"

#include "../Yggdrasil/Yggdrasil/protocols/utility/topologyManager.h"
#include "utility/myUtils.h"

#include "protocols/discovery/topology_discovery.h"

static int appID;

static void processArgs(int argc, char** argv, topology_discovery_args* d_args, unsigned long* periodic_timer_ms, bool* use_overlay, char* overlay_path);
static void printDiscoveryStats(YggRequest* req);
static void processNotification(YggEvent* notification);

int main(int argc, char* argv[]) {

	// Process Args
	topology_discovery_args d_args;
	new_topology_discovery_args(&d_args, 2, 1000, 5000, 60*1000, 5);
	unsigned long periodic_timer_ms = 10*1000;
	bool use_overlay = false;
	char overlay_path[PATH_MAX]; bzero(overlay_path, PATH_MAX);
	processArgs(argc, argv, &d_args, &periodic_timer_ms, &use_overlay, overlay_path);

	char* type = "AdHoc"; //should be an argument
	NetworkConfig* ntconf = defineNetworkConfig(type, 2462, 2, 1, "pis", YGG_filter);

	// Initialize ygg_runtime
	ygg_runtime_init(ntconf);

	// Register this app
	appID = 400;
	app_def* myApp = create_application_definition(appID, "Topology Discovery Test App");

	if(use_overlay) {
		// Register Topology Manager Protocol
		char db_file_path[PATH_MAX];
		char neighs_file_path[PATH_MAX];
		buildPath(db_file_path, overlay_path, "macAddrDB.txt");
		buildPath(neighs_file_path, overlay_path, "neighs.txt");
		int db_size = 24;// TODO: Como inferir o 24?
		topology_manager_args* args = topology_manager_args_init(db_size, db_file_path, neighs_file_path, true);
		registerYggProtocol(PROTO_TOPOLOGY_MANAGER, topologyManager_init, args);
		topology_manager_args_destroy(args);
	}

	// Register Topology Discovery Protocol
	registerProtocol(TOPOLOGY_DISCOVERY_PROTOCOL, &topology_discovery_init, (void*) &d_args);
	app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBOR_FOUND);
	app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBOR_UPDATE);
	app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBOR_LOST);
	app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTOCOL, NEIGHBORHOOD_UPDATE);

	queue_t* inBox = registerApp(myApp);

	// Start ygg_runtime
	ygg_runtime_start();

	////////////////////////////////////////////////////////////////////////////

	// Set periodic timer
	YggTimer timer;
	struct timespec t;
	milli_to_timespec(&t, periodic_timer_ms);
	YggTimer_init(&timer, appID, appID);
	YggTimer_set(&timer, t.tv_sec, t.tv_nsec, t.tv_sec, t.tv_nsec);
	setupTimer(&timer);

	// Prepare Stats Request
	YggRequest discovery_stats_req;
	YggRequest_init(&discovery_stats_req, appID, TOPOLOGY_DISCOVERY_PROTOCOL, REQUEST, STATS_REQ);

	queue_t_elem elem;
	while(true) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:
			deliverRequest(&discovery_stats_req);
			//printNeighbours();
			break;
		case YGG_REQUEST:
			if( elem.data.request.proto_dest == appID ) {
				if( elem.data.request.proto_origin == TOPOLOGY_DISCOVERY_PROTOCOL ) {
					if(elem.data.request.request == REPLY) {
						printDiscoveryStats(&elem.data.request);
					}
				}
			}
			YggRequest_freePayload(&elem.data.request);
			break;
			/*case YGG_MESSAGE:
			break;*/
		case YGG_EVENT:
			if( elem.data.event.proto_dest == appID ) {
				processNotification(&elem.data.event);

				// printNeighbors();
			} else {
				char s[100];
				sprintf(s, "Received notification from protocol %d meant for protocol %d", elem.data.event.proto_origin, elem.data.event.proto_dest);
				ygg_log("BCAST TEST APP", "NOTIFICATION", s);
			}
			break;
		default:
			ygg_log("DISCOVERY TEST APP", "MAIN LOOP", "Received wierd thing in my queue.");
		}

		free_elem_payload(&elem);
	}

	return 0;
}

static void processNotification(YggEvent* notification) {

	char id[UUID_STR_LEN];
	int h = ((unsigned char*)notification->payload)[0];
	unsigned char* ptr = 1 + (h+1) + notification->payload;

	uuid_unparse(ptr, id);

	if(notification->notification_id == NEIGHBOR_FOUND) {
		// TODO:
		ygg_log("DISCOVERY TEST APP", "NEIGHBOR FOUND", id);
	} else if(notification->notification_id == NEIGHBOR_UPDATE) {
		// TODO:
		ygg_log("DISCOVERY TEST APP", "NEIGHBOR UPDATE", id);
	} else if(notification->notification_id == NEIGHBOR_LOST) {
		// TODO:
		ygg_log("DISCOVERY TEST APP", "NEIGHBOR LOST", id);
	} else if(notification->notification_id == NEIGHBORHOOD_UPDATE) {
		// TODO:
		ygg_log("DISCOVERY TEST APP", "NEIGHBORHOOD UPDATE", id);
	}
}

static void printDiscoveryStats(YggRequest* req) {
	topology_discovery_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(stats));

	char str[200];
	sprintf(str, "Announces: %d \t Piggybacked: %d \t Lost Neighs: %d \t New Neighs: %d\n", stats.announces, stats.piggybacked_heartbeats, stats.lost_neighbours, stats.new_neighbours);
	ygg_log("DISCOVERY TEST APP", "STATS", str);
}

static void processArgs(int argc, char** argv, topology_discovery_args* d_args, unsigned long* periodic_timer_ms, bool* use_overlay, char* overlay_path) {

	struct option long_options[] = {
			{ "horizon", required_argument, NULL, 'h' },
			{ "max_jitter", required_argument, NULL, 'j' },
			{ "heartbeat_period", required_argument, NULL, 'p' },
			{ "t_gc", required_argument, NULL, 'g' },
			{ "misses", required_argument, NULL, 'm' },
			{ "periodic_timer", required_argument, NULL, 'l' },
			{ "overlay", required_argument, NULL, 'o' },
			{ NULL, 0, NULL, 0 }
	};

	int option_index = 0;
	int ch;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, "h:j:p:g:m:l:o:", long_options, &option_index)) != -1) {

		printf("%s: %s\n", long_options[option_index].name, optarg);

		switch (ch) {
		case 'h':
			d_args->horizon = (unsigned int) strtol(optarg, NULL, 10);

			break;
		case 'j':
			d_args->max_jitter = (unsigned long) strtol(optarg, NULL, 10);

			break;
		case 'p':
			d_args->heartbeat_period = (unsigned long) strtol(optarg, NULL, 10);

			break;
		case 'g':
			d_args->t_gc = (unsigned long) strtol(optarg, NULL, 10);

			break;
		case 'm':
			d_args->misses = (unsigned int) strtol(optarg, NULL, 10);

			break;
		case 'l':
			*periodic_timer_ms = (unsigned long) strtol(optarg, NULL, 10);

			break;
		case 'o':
			*use_overlay = true;
			memcpy(overlay_path, optarg, strlen(optarg));
			break;
		case '?':
			if ( optopt == 'h' || optopt == 'j' || optopt == 'p' || optopt == 'g' || optopt == 'm' || optopt == 'l' || optopt == 'o' )
				fprintf(stderr, "Option -%c is followed by a parameter.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			break;
		}
	}

}
