#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <jack/jack.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <glut.h>

#include "ringbuffer.h"
#include "polynomial.h"
#include "iirfilter.h"
#include "common.h"

#define LOW_MID_FREQ 200
#define MID_HIGH_FREQ 800

//declare opengl callbacks
void ChangeSize (int w, int h);
void SetupScene ();
void SpecialKeys (int key, int x, int y);
void RenderScene ();

jack_port_t *input_port;
jack_port_t *low_port, *mid_port, *high_port;
jack_client_t *client;

int type = TYPE_LOW;

typedef jack_default_audio_sample_t sample_t;
const double PI = 3.14159265358979323846;

//iir parameters
double cutoff = 400; //hz
double cutoff2 = 5000;
double analog_cutoff, analog_cutoff2;
int order = 8;
int highestROrder = 1;

double brightness[3];

jack_nframes_t sr;

iirfilter_t *dccutter;
iirfilter_t *lowfilter, *midfilter, *highfilter;
iirfilter_t *brightnessFilters[3];

//opengl parameters
static GLfloat xRot = 0.0f;
static GLfloat yRot = 0.0f;


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
    sample_t dc_removed =  next_sample (dccutter, in[i]);
    low[i] = next_sample (lowfilter, dc_removed);
    mid[i] = next_sample (midfilter, dc_removed);
    high[i] = next_sample (highfilter, dc_removed);
    brightness[0] = next_sample (brightnessFilters[0], 10 * fabs(low[i]));
    brightness[1] = next_sample (brightnessFilters[1], 10 * fabs(mid[i]));
    brightness[2] = next_sample (brightnessFilters[2], 10 * fabs(high[i]));
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
  printf ("the sample rate is now %lu/sec\n", nframes);
  sr=nframes;
  return 0;
}

int main (int argc, char *argv[])
{

  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize (800, 600);
  glutCreateWindow ("Threebands");
  glutReshapeFunc (ChangeSize);
  glutSpecialFunc (SpecialKeys);
  glutDisplayFunc (RenderScene);
  glutIdleFunc (RenderScene);
  SetupScene ();

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

  /* display the current sample rate. 
   */

  printf ("engine sample rate: %" PRIu32 "\n",
	  jack_get_sample_rate (client));
  sr=jack_get_sample_rate (client);

  analog_cutoff = tan (PI * cutoff / sr);
  analog_cutoff2 = tan (PI * cutoff2 / sr);
	
  dccutter = create_iirfilter (2, TYPE_HIGH, tan (PI / sr), analog_cutoff2); //get rid of any dc
  lowfilter = create_iirfilter (6, TYPE_LOW, tan (PI * LOW_MID_FREQ / sr), 0.0);
  midfilter = create_iirfilter (6, TYPE_BAND, tan (PI * LOW_MID_FREQ / sr), tan (PI * MID_HIGH_FREQ / sr));
  highfilter = create_iirfilter (6, TYPE_HIGH, tan (PI * MID_HIGH_FREQ / sr), 0.0);
  for (int i = 0; i < 3; i++)
    {
      brightnessFilters [i] = create_iirfilter (4, TYPE_LOW, tan (PI * 5 / sr), 0.0);
    }
  
  /* create two ports */
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



  /* keep running until stopped by the user */
  glutMainLoop();


  /* this is never reached but if the program
     had some other way to exit besides being killed,
     they would be important to call.
  */

  destroy_iirfilter (dccutter);
  destroy_iirfilter (lowfilter);
  destroy_iirfilter (midfilter);
  destroy_iirfilter (highfilter);
	
  jack_client_close (client);
  exit (0);
}

void ChangeSize (int w, int h)
{
  GLfloat fAspect;
  
  if (h == 0) h = 1;

  glViewport (0, 0, w, h);
  fAspect = (GLfloat)w / h;
  
  // Reset coordinate system
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  //and make the perspcetive
  gluPerspective (60.0f, fAspect, 1.0, 400);
  
  //state should be MODELVIEW by default
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
}

// This function does any needed initialization on the rendering
// context.  Here it sets up and initializes the lighting for
// the scene.
void SetupScene()
{
  // Light values and coordinates
  GLfloat  whiteLight[] = { 0.45f, 0.45f, 0.45f, 1.0f };
  GLfloat  sourceLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };
  GLfloat	 lightPos[] = { -50.f, 25.0f, 250.0f, 0.0f };

  glEnable(GL_DEPTH_TEST);	// Hidden surface removal
  glFrontFace(GL_CCW);		// Counter clock-wise polygons face out
  //glEnable(GL_CULL_FACE);		// Do not calculate inside of jet

  // Enable lighting
  glEnable(GL_LIGHTING);

  // Setup and enable light 0
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT,whiteLight);
  glLightfv(GL_LIGHT0,GL_AMBIENT,sourceLight);
  glLightfv(GL_LIGHT0,GL_DIFFUSE,sourceLight);
  glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
  glEnable(GL_LIGHT0);

  // Enable color tracking
  glEnable(GL_COLOR_MATERIAL);
	
  // Set Material properties to follow glColor values
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

  // Black blue background
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
}


// Respond to arrow keys
void SpecialKeys(int key, int x, int y)
{
  if(key == GLUT_KEY_UP)
    xRot-= 5.0f;

  if(key == GLUT_KEY_DOWN)
    xRot += 5.0f;

  if(key == GLUT_KEY_LEFT)
    yRot -= 5.0f;

  if(key == GLUT_KEY_RIGHT)
    yRot += 5.0f;
                
  xRot = (GLfloat)((const int)xRot % 360);
  yRot = (GLfloat)((const int)yRot % 360);

  // Refresh the Window
  glutPostRedisplay();
}

// Called to draw scene
void RenderScene(void)
{
  float fZ,bZ;

  // Clear the window with current clearing color
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  fZ = 100.0f;
  bZ = -100.0f;

  // Save the matrix state and do the rotations
  glPushMatrix();
  glTranslatef(0.0f, 0.0f, -300.0f);
  glRotatef(xRot, 1.0f, 0.0f, 0.0f);
  glRotatef(yRot, 0.0f, 1.0f, 0.0f);

  // Set material color, Red
  glBegin (GL_QUADS);
  glNormal3f (0.0f, 0.0f, 1.0f);
  
  //left
  glColor3f (1.0f, 0.0f, 0.0f);
  glVertex3f (-50, -100, fZ);
  glVertex3f (-30, -100, fZ);
  float leftHeight = 100 * brightness [0];
  glVertex3f (-30, leftHeight - 100, fZ);
  glVertex3f (-50, leftHeight - 100, fZ);

  //mid
  glColor3f (0.0f, 1.0f, 0.0f);
  glVertex3f (0, -100, fZ);
  glVertex3f (20, -100, fZ);
  float midHeight = 100 * brightness [1];
  glVertex3f (20, midHeight - 100, fZ);
  glVertex3f (0, midHeight - 100, fZ);

  //left
  glColor3f (0.0f, 0.0f, 1.0f);
  glVertex3f (50, -100, fZ);
  glVertex3f (70, -100, fZ);
  float rightHeight = 100 * brightness [2];
  glVertex3f (70, rightHeight - 100, fZ);
  glVertex3f (50, rightHeight - 100, fZ);

  glEnd ();


  
  /*
  // Front Face ///////////////////////////////////
  glBegin(GL_QUADS);
  // Pointing straight out Z
  glNormal3f(0.0f, 0.0f, 1.0f);	

  // Left Panel
  glVertex3f(-50.0f, 50.0f, fZ);
  glVertex3f(-50.0f, -50.0f, fZ);
  glVertex3f(-35.0f, -50.0f, fZ);
  glVertex3f(-35.0f,50.0f,fZ);

  // Right Panel
  glVertex3f(50.0f, 50.0f, fZ);
  glVertex3f(35.0f, 50.0f, fZ);
  glVertex3f(35.0f, -50.0f, fZ);
  glVertex3f(50.0f,-50.0f,fZ);

  // Top Panel
  glVertex3f(-35.0f, 50.0f, fZ);
  glVertex3f(-35.0f, 35.0f, fZ);
  glVertex3f(35.0f, 35.0f, fZ);
  glVertex3f(35.0f, 50.0f,fZ);

  // Bottom Panel
  glVertex3f(-35.0f, -35.0f, fZ);
  glVertex3f(-35.0f, -50.0f, fZ);
  glVertex3f(35.0f, -50.0f, fZ);
  glVertex3f(35.0f, -35.0f,fZ);

  // Top length section ////////////////////////////
  // Normal points up Y axis
  glNormal3f(0.0f, 1.0f, 0.0f);
  glVertex3f(-50.0f, 50.0f, fZ);
  glVertex3f(50.0f, 50.0f, fZ);
  glVertex3f(50.0f, 50.0f, bZ);
  glVertex3f(-50.0f,50.0f,bZ);
		
  // Bottom section
  glNormal3f(0.0f, -1.0f, 0.0f);
  glVertex3f(-50.0f, -50.0f, fZ);
  glVertex3f(-50.0f, -50.0f, bZ);
  glVertex3f(50.0f, -50.0f, bZ);
  glVertex3f(50.0f, -50.0f, fZ);

  // Left section
  glNormal3f(1.0f, 0.0f, 0.0f);
  glVertex3f(50.0f, 50.0f, fZ);
  glVertex3f(50.0f, -50.0f, fZ);
  glVertex3f(50.0f, -50.0f, bZ);
  glVertex3f(50.0f, 50.0f, bZ);

  // Right Section
  glNormal3f(-1.0f, 0.0f, 0.0f);
  glVertex3f(-50.0f, 50.0f, fZ);
  glVertex3f(-50.0f, 50.0f, bZ);
  glVertex3f(-50.0f, -50.0f, bZ);
  glVertex3f(-50.0f, -50.0f, fZ);
  glEnd();

  glFrontFace(GL_CW);		// clock-wise polygons face out

  glBegin(GL_QUADS);
  // Back section
  // Pointing straight out Z
  glNormal3f(0.0f, 0.0f, -1.0f);	

  // Left Panel
  glVertex3f(-50.0f, 50.0f, bZ);
  glVertex3f(-50.0f, -50.0f, bZ);
  glVertex3f(-35.0f, -50.0f, bZ);
  glVertex3f(-35.0f,50.0f,bZ);

  // Right Panel
  glVertex3f(50.0f, 50.0f, bZ);
  glVertex3f(35.0f, 50.0f, bZ);
  glVertex3f(35.0f, -50.0f, bZ);
  glVertex3f(50.0f,-50.0f,bZ);

  // Top Panel
  glVertex3f(-35.0f, 50.0f, bZ);
  glVertex3f(-35.0f, 35.0f, bZ);
  glVertex3f(35.0f, 35.0f, bZ);
  glVertex3f(35.0f, 50.0f,bZ);

  // Bottom Panel
  glVertex3f(-35.0f, -35.0f, bZ);
  glVertex3f(-35.0f, -50.0f, bZ);
  glVertex3f(35.0f, -50.0f, bZ);
  glVertex3f(35.0f, -35.0f,bZ);
	
  // Insides /////////////////////////////
  glColor3f(0.75f, 0.75f, 0.75f);

  // Normal points up Y axis
  glNormal3f(0.0f, 1.0f, 0.0f);
  glVertex3f(-35.0f, 35.0f, fZ);
  glVertex3f(35.0f, 35.0f, fZ);
  glVertex3f(35.0f, 35.0f, bZ);
  glVertex3f(-35.0f,35.0f,bZ);
		
  // Bottom section
  glNormal3f(0.0f, 1.0f, 0.0f);
  glVertex3f(-35.0f, -35.0f, fZ);
  glVertex3f(-35.0f, -35.0f, bZ);
  glVertex3f(35.0f, -35.0f, bZ);
  glVertex3f(35.0f, -35.0f, fZ);

  // Left section
  glNormal3f(1.0f, 0.0f, 0.0f);
  glVertex3f(-35.0f, 35.0f, fZ);
  glVertex3f(-35.0f, 35.0f, bZ);
  glVertex3f(-35.0f, -35.0f, bZ);
  glVertex3f(-35.0f, -35.0f, fZ);

  // Right Section
  glNormal3f(-1.0f, 0.0f, 0.0f);
  glVertex3f(35.0f, 35.0f, fZ);
  glVertex3f(35.0f, -35.0f, fZ);
  glVertex3f(35.0f, -35.0f, bZ);
  glVertex3f(35.0f, 35.0f, bZ);
  glEnd();*/

  glFrontFace(GL_CCW);		// Counter clock-wise polygons face out

  // Restore the matrix state
  glPopMatrix();

  // Buffer swap
  glutSwapBuffers();
}

