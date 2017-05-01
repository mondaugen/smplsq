#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 
#include <pthread.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h> 

#include <jack/jack.h>

#include "defs.h" 
#include "types.h"
#include "synth.h"
#include "seq.h"

#define MYPORT "4950"	// the port users will be connecting to

#define MAXBUFLEN 100

#define WAVETABLE_LEN 4096
#define WAVETABLE_NHARM 10 

static volatile int done = 0;

void sigintfun(int signum) { done = 1; }

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

typedef struct synth_vc_init_list_t {
    synth_vc_init_t head;
    struct synth_vc_init_list_t *next;
} synth_vc_init_list_t;

static volatile synth_vc_init_list_t *svci = NULL,
                                     *svci_used = NULL;

synth_vc_proc_t synthproc;

pthread_mutex_t voice_mutex;

//jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;


static void wavetable_init(f64_t *wt, size_t len, size_t nharm)
{
    /* Initializes with harmonic series */
    size_t n, m;
    _MZ(wt,f64_t,len);
    for (n = 1; n <= nharm; n++) {
        f64_t phs_inc = 2. * M_PI * (f64_t)n / len,
              phs = 0;
        for (m = 0; m < len; m++) {
            wt[m] += cos(phs) / (f64_t)(n*n);
            phs += phs_inc;
        }
    }
}

static void parse_mess(char *buf)
{
    char *sep1 = " ", *sep2 = "\n"
         *lasts;
    strtok_r(buf,sep1,&lasts);
    //if (strcmp(buf,"clear") == 0) {
    //    seq_remove_all_events(seq);
    //}
    if (strcmp(buf,"note") == 0) {
        synth_vc_init_list_t *tmp = _C(synth_vc_init_list_t,1);
        if (!tmp) { return; }
        strtok_r(NULL,sep2,&lasts);
        if (synth_vc_init_from_str((synth_vc_init_t*)tmp,lasts)
                != err_NONE) {
            _F(tmp);
        }
        tmp->next = svci;
        svci = tmp;
    }
}

int
process (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out;
//	jack_default_audio_sample_t *in, *out;
	
//	in = jack_port_get_buffer (input_port, nframes);
    /* Go through init structs and activate synths if they are available. */
    size_t n;
    if (pthread_mutex_trylock(&voice_mutex)) {
        while (svci) {
            /* find free voice */
            int free_voice = -1;
            for (n = 0; n < nvoices; n++) {
                if (!voices[n]->playing) {
                    free_voice = n;
                    n = nvoices; /* break for loop */
                }
            }
            if (free_voice >= 0) {
                /* activate this synth */
                synth_vc_init(&voices[free_voice],
                        (synth_vc_init_t*)svci);
            }
            /* if wasn't activated, we still discard it */
            synth_vc_init_list_t *tmp = svci->next;
            svci->next = svci_used;
            svci_used = svci;
            svci = tmp;
        }
        pthread_mutex_unlock(&voice_mutex);
    }
	out = jack_port_get_buffer (output_port, nframes);
    _MZ(wt,jack_default_audio_sample_t,nframes);
    for (n = 0; n < nvoices; n++) {
        if (voices[n]->playing) {
            synth_vc_proc(&voices[n],&synthproc,out,nframes);
        }
    }
	return 0;      
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

int
main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name = "simple";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	
	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate. 
	 */

	printf ("engine sample rate: %" PRIu32 "\n",
		jack_get_sample_rate (client));

    f64_t *wt = _M(f64_t,WAVETABLE_LEN);
    wavetable_init(wt,WAVETABLE_LEN,WAVETABLE_NHARM);
    synthproc = (synth_vc_proc_t) {
        .sr = (f64_t)jack_get_sample_rate(client);
        .wt = wt;
        .len = WAVETABLE_LEN;
    };

	/* create two ports */

	//input_port = jack_port_register (client, "input",
	//				 JACK_DEFAULT_AUDIO_TYPE,
	//				 JackPortIsInput, 0);
	output_port = jack_port_register (client, "output",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

//	if ((input_port == NULL) || (output_port == NULL)) {
	if ((output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	//ports = jack_get_ports (client, NULL, NULL,
	//			JackPortIsPhysical|JackPortIsOutput);
	//if (ports == NULL) {
	//	fprintf(stderr, "no physical capture ports\n");
	//	exit (1);
	//}

	//if (jack_connect (client, ports[0], jack_port_name (input_port))) {
	//	fprintf (stderr, "cannot connect input ports\n");
	//}

	//free (ports);
	
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

    signal(SIGINT,sigintfun);
	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;

    pthread_mutex_init(&voice_mutex);
    while (!done) {
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        printf("listener: got packet from %s\n",
                inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof s));
        printf("listener: packet is %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        pthread_mutex_lock(&voice_mutex);
        /* free any used svci */
        while (svci_used) {
            synth_vc_init_list_t *tmp = svci_used->next;
            _F(svci_used);
            svci_used = tmp;
        }
        parse_mess(buf);
        pthread_mutex_unlock(&voice_mutex);
    }

	close(sockfd);
	jack_client_close (client);
	exit (0);
}

