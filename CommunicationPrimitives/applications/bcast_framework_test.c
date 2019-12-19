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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <linux/limits.h>

#include "Yggdrasil_lowlvl.h"
#include "../Yggdrasil/Yggdrasil/core/utils/queue.h"
#include "../Yggdrasil/Yggdrasil/core/ygg_runtime.h"

#include "../Yggdrasil/Yggdrasil/protocols/utility/topologyManager.h"

#include "protocols/broadcast/framework/framework.h"
#include "utility/myUtils.h"

static void processArgs(int argc, char** argv, framework_args* f_args, unsigned long* initial_timer_ms, unsigned long* periodic_timer_ms, unsigned int* max_broadcasts, unsigned long* exp_duration_s, unsigned char* rcv_only, double* bcast_prob, bool* use_overlay, char* overlay_path);
static void mountBroadcastAlgorithm(char* token, BroadcastAlgorithm* bcast_alg);
static void mountRetransmissionDelay(char* token, BroadcastAlgorithm* bcast_alg);
static void mountRetransmissionPolicy(char* token, BroadcastAlgorithm* bcast_alg);
static void mountRetransmissionContext(char* token, BroadcastAlgorithm* bcast_alg);

//static void printDiscoveryStats(YggRequest* req);
static void printBcastStats(YggRequest* req);
static void bcastMessage(YggRequest* framework_bcast_req, const char* pi, unsigned int counter);
static void rcvMessage(YggMessage* msg);
static void uponNotification(YggEvent* ev);

static char* getPath(char* file, char* folder, char* file_name) {
	bzero(file, PATH_MAX);

	strcpy(file, folder);
	if(file[strlen(file)] != '/')
		strcat(file, "/");
	strcat(file, file_name);

	return file;
}

static int appID;

int main(int argc, char* argv[]) {

	// TODO: Meter para ler estas configs de um ficheiro em vez de ser apenas args

	// Process Args
	framework_args f_args;
	f_args.algorithm = Flooding(1000);
	f_args.gc_interval_s = 3*60;  // 5 min
	f_args.seen_expiration_ms = 1*60*1000; // 1 min
	unsigned long initial_timer_ms = 1000;
	unsigned long periodic_timer_ms = 1000;
	unsigned int max_broadcasts = UINT32_MAX; // TODO: ter opção para não ter este limite sequer
	unsigned long exp_duration_s = 5*60; // 5 min // TODO: ter opção para não ter este limite
	unsigned char rcv_only = 0;
	double bcast_prob = 1.0;
	bool use_overlay = false;
	char overlay_path[PATH_MAX]; bzero(overlay_path, PATH_MAX);
	processArgs(argc, argv, &f_args, &initial_timer_ms, &periodic_timer_ms, &max_broadcasts, &exp_duration_s, &rcv_only, &bcast_prob, &use_overlay, overlay_path);

	char* type = "AdHoc"; //should be an argument
	NetworkConfig* ntconf = defineNetworkConfig(type, 2462, 2, 1, "pis", YGG_filter);

	appID = 400;

	// Initialize ygg_runtime
	ygg_runtime_init(ntconf);

	// Register this app
	app_def* myApp = create_application_definition(appID, "Bcast Framework Test App");
	queue_t* inBox = registerApp(myApp);

	if(use_overlay) {
		// Register Topology Manager Protocol
		char db_file_path[PATH_MAX];
		char neighs_file_path[PATH_MAX];
		getPath(db_file_path, overlay_path, "macAddrDB.txt");
		getPath(neighs_file_path, overlay_path, "neighs.txt");
		int db_size = 24;// TODO: Como inferir o 24?
		topology_manager_args* args = topology_manager_args_init(db_size, db_file_path, neighs_file_path, true);
		registerYggProtocol(PROTO_TOPOLOGY_MANAGER, topologyManager_init, args);
		topology_manager_args_destroy(args);
	}

	// Register Framework Protocol
	registerProtocol(BCAST_FRAMEWORK_PROTO_ID, &framework_init, (void*) &f_args);

	//app_def_add_consumed_events(myApp, MY_DISCOVERY_PROTO, DISC_NEIGHBOR_FOUND);
	//app_def_add_consumed_events(myApp, MY_DISCOVERY_PROTO, DISC_NEIGHBOR_LOST);

	// Start ygg_runtime
	ygg_runtime_start();

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Start App

	// Set periodic timer
	YggTimer timer;
	struct timespec t, init;
	milli_to_timespec(&t, periodic_timer_ms);
	milli_to_timespec(&init, initial_timer_ms);
	YggTimer_init(&timer, appID, appID);
	YggTimer_set(&timer, init.tv_sec, init.tv_nsec, t.tv_sec, t.tv_nsec);
	setupTimer(&timer);

	// Set end of experiment
	uuid_t end_exp;
	genUUID(end_exp);
	YggTimer_init_with_uuid(&timer, end_exp, appID, appID);
	milli_to_timespec(&t, exp_duration_s*1000);
	YggTimer_set(&timer, t.tv_sec, t.tv_nsec, 0, 0);
	setupTimer(&timer);

	/*YggRequest discovery_req;
	YggRequest_init(&discovery_req, appID, MY_DISCOVERY_PROTO, REQUEST, STATS_REQ);*/
	YggRequest framework_stats_req;
	YggRequest_init(&framework_stats_req, appID, BCAST_FRAMEWORK_PROTO_ID, REQUEST, FRAMEWORK_STATS_REQ);
	YggRequest framework_bcast_req;
	YggRequest_init(&framework_bcast_req, appID, BCAST_FRAMEWORK_PROTO_ID, REQUEST, FRAMEWORK_BCAST_MSG_REQ);

	const char* pi = getHostname(); // raspi-n

	char str[100];
	sprintf(str, "%s starting experience of %lu s\n", pi, exp_duration_s);
	ygg_log("APP", "INIT", str);

	unsigned int counter = 0;
	//YggMessage msg;

	queue_t_elem elem;
	while(1) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:
			// Send new msg with probability send_prob
			if( uuid_compare(elem.data.timer.id, end_exp) == 0) {
				//cancelTimer(&elem.data.timer);
				rcv_only = true;
			} else {
				if (!rcv_only) {
					if( counter < max_broadcasts ) {
						if ( bcast_prob == 1.0 || bcast_prob >= getRandomProb()) {
							bcastMessage(&framework_bcast_req, pi, ++counter);
						}
					}
				}
			}

			//deliverRequest(&discovery_req);
			deliverRequest(&framework_stats_req);
			//printNeighbours();

			YggTimer_freePayload(&elem.data.timer);
			break;
		case YGG_REQUEST:
			if( elem.data.request.proto_dest == appID ) {
				/*if( elem.data.request.proto_origin == MY_DISCOVERY_PROTO ) {
					if(elem.data.request.request == REPLY) {
						printDiscoveryStats(&elem.data.request);
					}
				} else*/ if ( elem.data.request.proto_origin == BCAST_FRAMEWORK_PROTO_ID ) {
					if(elem.data.request.request == REPLY && elem.data.request.request_type == FRAMEWORK_STATS_REQ) {
						printBcastStats(&elem.data.request);
					}
				}
			}
			YggRequest_freePayload(&elem.data.request);
			break;
		case YGG_MESSAGE:
			rcvMessage(&elem.data.msg);
			break;
		case YGG_EVENT:
			uponNotification(&elem.data.event);
			YggEvent_freePayload(&elem.data.event);
			break;
		default:
			ygg_log("APP", "MAIN LOOP", "Received wierd thing in my queue.");
		}
	}

	return 0;
}

static void processArgs(int argc, char** argv, framework_args* f_args, unsigned long* initial_timer_ms, unsigned long* periodic_timer_ms, unsigned int* max_broadcasts, unsigned long* exp_duration_s, unsigned char* rcv_only, double* bcast_prob, bool* use_overlay, char* overlay_path) {

	struct option long_options[] = {
			{ "r_alg", required_argument, NULL, 'a' },
			{ "r_delay", required_argument, NULL, 't' },
			{ "r_policy",required_argument, NULL, 'r' },
			{ "r_context", required_argument, NULL, 'c' },
			{ "r_phase", required_argument, NULL, 'n' },
			{ "rcv_only", no_argument, NULL, 'R' },
			{ "max_bcasts", required_argument, NULL, 'b' },
			{ "init_timer", required_argument, NULL, 'i' },
			{ "periodic_timer", required_argument, NULL, 'l' },
			{ "duration", required_argument, NULL, 'd' },
			{ "bcast_prob", required_argument, NULL, 'p' },
			{ "overlay", required_argument, NULL, 'o' },
			{ NULL, 0, NULL, 0 }
	};

	int option_index = 0;
	int ch;
	opterr = 0;
	char *token;
	while ((ch = getopt_long(argc, argv, "a:r:t:c:n:b:i:l:d:op:", long_options, &option_index)) != -1) {

		printf("%s: %s\n", long_options[option_index].name, optarg);

		switch (ch) {
		case 'a':
			token = strtok(optarg, " ");

			if(token != NULL) {
				mountBroadcastAlgorithm(token, &f_args->algorithm);
			} else
				printf("No parameter passed");

			break;
		case 'r':
			token = strtok(optarg, " ");

			if(token != NULL) {
				mountRetransmissionPolicy(token, &f_args->algorithm);
			} else
				printf("No parameter passed");

			break;
		case 't':
			token = strtok(optarg, " ");

			if(token != NULL) {
				mountRetransmissionDelay(token, &f_args->algorithm);
			} else
				printf("No parameter passed");

			break;
		case 'c':
			token = strtok(optarg, " ");

			if(token != NULL) {
				mountRetransmissionContext(token, &f_args->algorithm);
			} else
				printf("No parameter passed");

			break;
		case 'n':
			f_args->algorithm.n_phases = strtol(optarg, NULL, 10);
			//printf(" %d\n", bcast_alg->n_phases);

			break;
		case 'b':
			*max_broadcasts = (unsigned int) strtol(optarg, NULL, 10);
			//printf(" %d\n", max_bcasts);

			break;
		case 'i':
			*initial_timer_ms = (unsigned long) strtol(optarg, NULL, 10);
			//printf(" %lu\n", init);

			break;
		case 'l':
			*periodic_timer_ms = (unsigned long) strtol(optarg, NULL, 10);
			//printf(" %lu\n", periodic);

			break;
		case 'd':
			*exp_duration_s = (unsigned long) strtol(optarg, NULL, 10);
			//printf(" %lu\n", duration);

			break;
		case 'R':
			*rcv_only = true;

			break;
		case 'p':
			*bcast_prob = strtod (optarg, NULL);
			//printf(" %f\n", prob);
			break;
		case 'o':
			*use_overlay = true;
			memcpy(overlay_path, optarg, strlen(optarg));
			break;
		case '?':
			if ( optopt == 'a' || optopt == 'r' || optopt == 't' || optopt == 'c' || optopt == 'n' || optopt == 'b' || optopt == 'i' || optopt == 'l' || optopt == 'd' || optopt == 'p')
				fprintf(stderr, "Option -%c is followed by a parameter.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			break;
		}

		//printf("-s = %d, -t = %d\n", *silent_mode, *m_sleep);

		/*for (int index = optind; index < argc; index++)
	 printf("Non-option argument %s\n", argv[index]);*/
	}
}

static void mountBroadcastAlgorithm(char* token, BroadcastAlgorithm* bcast_alg) {
	char* name;
	if(strcmp(token, (name = "Flooding")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);
			//printf(" %lu\n", max_delay);

			*bcast_alg = Flooding(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip1")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double p = strtod (token, NULL);

				*bcast_alg = Gossip1(t, p);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip1_hops")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double p = strtod (token, NULL);

				token = strtok(NULL, " ");
				if(token != NULL) {
					unsigned int k = strtol(token, NULL, 10);

					*bcast_alg = Gossip1_hops(t, p, k);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip2")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double p1 = strtod (token, NULL);

				token = strtok(NULL, " ");
				if(token != NULL) {
					unsigned int k = strtol(token, NULL, 10);

					token = strtok(NULL, " ");
					if(token != NULL) {
						double p2 = strtod (token, NULL);

						token = strtok(NULL, " ");
						if(token != NULL) {
							unsigned int n = strtol(token, NULL, 10);

							*bcast_alg = Gossip2(t, p1, k, p2, n);
						} else {
							printf("Parameter 5 of %s not passed!\n", name);
							exit(-1);
						}
					} else {
						printf("Parameter 4 of %s not passed!\n", name);
						exit(-1);
					}
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip3")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				unsigned long t2 = strtol(token, NULL, 10);

				token = strtok(NULL, " ");
				if(token != NULL) {
					double p = strtod (token, NULL);

					token = strtok(NULL, " ");
					if(token != NULL) {
						unsigned int k = strtol(token, NULL, 10);

						token = strtok(NULL, " ");
						if(token != NULL) {
							unsigned int m = strtol(token, NULL, 10);

							*bcast_alg = Gossip3(t1, t2, p, k, m);
						} else {
							printf("Parameter 5 of %s not passed!\n", name);
							exit(-1);
						}
					} else {
						printf("Parameter 4 of %s not passed!\n", name);
						exit(-1);
					}
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Rapid")) == 0 || strcmp(token, (name = "RAPID")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double beta = strtod(token, NULL);

				*bcast_alg = RAPID(t, beta);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "EnhancedRapid")) == 0 || strcmp(token, (name = "EnhancedRAPID")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				unsigned long t2 = strtol(token, NULL, 10);

				token = strtok(NULL, " ");
				if(token != NULL) {
					double beta = strtod(token, NULL);

					*bcast_alg = EnhancedRAPID(t1, t2, beta);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Counting")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				unsigned int c = strtol(token, NULL, 10);

				*bcast_alg = Counting(t, c);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HopCountAided")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			*bcast_alg = HopCountAided(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA1")) == 0 || strcmp(token, (name = "CountingNABA")) == 0) {

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				unsigned int c = strtol(token, NULL, 10);

				*bcast_alg = NABA1(t, c);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA2")) == 0 || strcmp(token, (name = "PbCountingNABA")) == 0) {

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				unsigned int c1 = strtol(token, NULL, 10);

				token = strtok(NULL, " ");
				if(token != NULL) {
					unsigned int c2 = strtol(token, NULL, 10);

					*bcast_alg = NABA2(t, c1, c2);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA3")) == 0) {

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double min_critical_coverage = strtod(token, NULL);

				*bcast_alg = NABA3(t, min_critical_coverage);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA4")) == 0) {

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double min_critical_coverage = strtod(token, NULL);

				*bcast_alg = NABA4(t, min_critical_coverage);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "CriticalNABA")) == 0) {

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok(NULL, " ");
			if(token != NULL) {
				double min_critical_coverage = strtod(token, NULL);

				token = strtok(NULL, " ");
				if(token != NULL) {
					unsigned int np = strtol(token, NULL, 10);

					*bcast_alg = NABA3e4(t, min_critical_coverage, np);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else {
		printf("Unrecognized Bcast Algorithm! \n");
		exit(-1);
	}

}

static void mountRetransmissionDelay(char* token, BroadcastAlgorithm* bcast_alg) {
	char* name;
	if(strcmp(token, (name = "Random")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);
			//printf(" %lu\n", max_delay);
			bcast_alg->r_delay = RandomDelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Neigh")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);
			//printf(" %lu\n", max_delay);
			bcast_alg->r_delay = DensityNeighDelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Null")) == 0) {
		bcast_alg->r_delay = NullDelay();
	} else {
		exit(-1);
		printf("Unrecognized R. Delay! \n");
	}
}

static void mountRetransmissionPolicy(char* token, BroadcastAlgorithm* bcast_alg) {
	char* name;
	if(strcmp(token, (name = "True")) == 0) {
		//printf(" %s\n", token);
		bcast_alg->r_policy = TruePolicy();
	} else if(strcmp(token, (name = "Probability")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			double p = strtod (token, NULL);

			bcast_alg->r_policy = ProbabilityPolicy(p);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Count")) == 0) {
		//printf(" %s\n", token);

		token = strtok(NULL, " ");
		if(token != NULL) {
			unsigned int c = strtol(token, NULL, 10);

			bcast_alg->r_policy = CountPolicy(c);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else {
		exit(-1);
		printf("Unrecognized R. Policy! \n");
	}
}

static void mountRetransmissionContext(char* token, BroadcastAlgorithm* bcast_alg) {
	const char* name = "Empty";
	if(strcmp(token, name) == 0) {
		bcast_alg->r_context = EmptyContext();
	} else {
		exit(-1);
		printf("Unrecognized R. Context! \n");
	}
}


static void bcastMessage(YggRequest* framework_bcast_req, const char* pi, unsigned int counter) {
	char payload[1000];
	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);
	//sprintf(payload, "I'm %s and this is my %dth message, sent at %ld:%ld.", pi, counter, current_time.tv_sec, current_time.tv_nsec);
	sprintf(payload, "I'm %s and this is my %dth message.", pi, counter);
	YggRequest_addPayload(framework_bcast_req, payload, strlen(payload)+1);
	deliverRequest(framework_bcast_req);
	YggRequest_freePayload(framework_bcast_req);
	ygg_log("BCAST TEST APP", "BROADCAST SENT", payload);
}

static void rcvMessage(YggMessage* msg) {
	char m[1000];
	memset(m, 0, 1000);

	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);

	if (msg->dataLen > 0 && msg->data != NULL) {
		memcpy(m, msg->data, msg->dataLen);
	} else {
		strcpy(m, "[EMPTY MESSAGE]");
	}
	//sprintf(m, "%s Received at %ld:%ld.", m, current_time.tv_sec, current_time.tv_nsec);
	ygg_log("BCAST TEST APP", "RECEIVED MESSAGE", m);
}

static void printBcastStats(YggRequest* req) {
	bcast_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(bcast_stats));

	char m[2000];
	memset(m, 0, 2000);
	sprintf(m, "Received: %d \t Sent: %d \t Not_Sent: %d \t Delivered: %d\n",
			stats.messages_received, stats.messages_sent,
			stats.messages_not_sent, stats.messages_delivered);

	ygg_log("BCAST TEST APP", "BCAST STATS", m);
}

/*static void printDiscoveryStats(YggRequest* req) {
	discovery_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(discovery_stats));

	char str[200];

	sprintf(str, "Announces: %d \t piggybacked: %d \t Lost Neighbours: %d \t newNeighbours: %d\n", stats.announce_counter, stats.passive_heartbeats, stats.lost_neighbours, stats.new_neighbours);
	ygg_log("BCAST TEST APP", "DISCOVERY STATS", str);

	// TODO: Fazer free do payload?
}*/

static void uponNotification(YggEvent* ev) {

	if( ev->proto_dest == appID ) {
		//printNeighbours();
	} else {
		char s[100];
		sprintf(s, "Received notification from protocol %d meant for protocol %d", ev->proto_origin, ev->proto_dest);
		ygg_log("BCAST TEST APP", "NOTIFICATION", s);
	}

}

