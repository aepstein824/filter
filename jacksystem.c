#include <unistd.h>
#include <stdio.h>
#include <jack/jack.h>

typedef jack_default_audio_sample_t sample_t;

#include "jacksystem.h"
#include "ThreebandSplitter.h"
#include "iirfilter.h"

static ThreebandSplitter *tbs;

jack_port_t *input_port;
jack_port_t *low_port, *mid_port, *high_port;
jack_client_t *client;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 */
int process (jack_nframes_t nframes, void *arg)
{
  jack_default_audio_sample_t *in, *low, *mid, *high;;
	
  in = jack_port_get_buffer (input_port, nframes);
  low = jack_port_get_buffer (low_port, nframes);
  mid = jack_port_get_buffer (mid_port, nframes);
  high = jack_port_get_buffer (high_port, nframes);

  for (int i = 0; i < nframes; i++){
    sample_t dc_removed =  next_sample (tbs -> dcCutter, in[i]);
    tbs -> avc = next_sample (tbs -> avcFilter, fabs (dc_removed));
    low[i] = next_sample (tbs -> lowFilter, dc_removed);
    mid[i] = next_sample (tbs -> midFilter, dc_removed);
    high[i] = next_sample (tbs -> highFilter, dc_removed);
    tbs -> levels[0] = next_sample (tbs -> levelFilters[0], fabs(low[i]));
    tbs -> levels[1] = next_sample (tbs -> levelFilters[1], fabs(mid[i]));
    tbs -> levels[2] = next_sample (tbs -> levelFilters[2], fabs(high[i]));
  }
	
  return 0;      
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg)
{
  exit (1);
}

int srate (jack_nframes_t nframes, void *arg){
  tbs -> sampleRate = nframes;
  calculateFilters (tbs);
  return 0;
}


void SetupJackSystem (ThreebandSplitter *t)
{
  tbs = t;
  
  const char **ports;
  const char *client_name = "threebands";
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

  jack_set_sample_rate_callback (client, srate, 0);

  tbs -> sampleRate = jack_get_sample_rate (client);


  input_port = jack_port_register (client, "input",
				   JACK_DEFAULT_AUDIO_TYPE,
				   JackPortIsInput, 0);
  low_port = jack_port_register (client, "low",
				 JACK_DEFAULT_AUDIO_TYPE,
				 JackPortIsOutput, 0);
  mid_port = jack_port_register (client, "mid",
				 JACK_DEFAULT_AUDIO_TYPE,
				 JackPortIsOutput, 0);
  high_port = jack_port_register (client, "high",
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput, 0);

  calculateFilters (tbs);

  if ((input_port == NULL) || (high_port == NULL)) {
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
   * that aren't running.
   */
  ports = jack_get_ports (client, NULL, NULL,
			  JackPortIsPhysical|JackPortIsOutput);
  if (ports == NULL) {
    fprintf(stderr, "no physical capture ports\n");
    exit (1);
  }
  if (jack_connect (client, ports[0], jack_port_name (input_port))) {
    fprintf (stderr, "cannot connect input ports\n");
  }
  free (ports);


  /*
   * if someone wants to listen in, they can, but we won't make it
   * automatic
   ports = jack_get_ports (client, NULL, NULL,
   JackPortIsPhysical|JackPortIsInput);
   if (ports == NULL) {
   fprintf(stderr, "no physical playback ports\n");
   exit (1);
   }
   if (jack_connect (client, jack_port_name (low_port), ports[0])) {
   fprintf (stderr, "cannot connect output ports\n");
   free (ports);
   }*/
}

void CloseJackSystem ()
{
  jack_client_close (client);
}
