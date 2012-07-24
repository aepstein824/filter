#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <jack/jack.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"

#include "ringbuffer.h"
#include "polynomial.h"
#include "iirfilter.h"
#include "common.h"

#define LOW_MID_FREQ 150
#define MID_HIGH_FREQ 750

#define AVC_FREQ .1
#define BRIGHTNESS_FREQ 5

//declare opengl callbacks
void ChangeSize (int w, int h);
void SetupScene ();
void Keyboard (unsigned char key, int x, int y);
void SpecialKeys (int key, int x, int y);
void RenderScene ();

jack_port_t *input_port;
jack_port_t *low_port, *mid_port, *high_port;
jack_client_t *client;

typedef jack_default_audio_sample_t sample_t;
const double PI = 3.14159265358979323846;

//iir parameters
double cutoff = 400; //hz
double cutoff2 = 5000;
double analog_cutoff, analog_cutoff2;
int order = 8;
int highestROrder = 1;

double avc;
double brightness[3];
double userBrightness[4] = {1., 1., 1., 1.};
float userBScaleMax = 10.0;
float userBScaleFactor = 1.25;

jack_nframes_t sr;

iirfilter_t *dccutter;
iirfilter_t *avcfilter;
iirfilter_t *lowfilter, *midfilter, *highfilter;
iirfilter_t *brightnessFilters[3];

//opengl parameters
static GLfloat xRot = 0.0f;
static GLfloat yRot = 0.0f;
int isRotating = 0;
clock_t startTick = 0;
//textures
GLuint prgTex[64][64*3];
GLuint texNum = 0;

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
    avc = next_sample (avcfilter, fabs (dc_removed));
    low[i] = next_sample (lowfilter, dc_removed);
    mid[i] = next_sample (midfilter, dc_removed);
    high[i] = next_sample (highfilter, dc_removed);
    brightness[0] = next_sample (brightnessFilters[0], fabs(low[i]));
    brightness[1] = next_sample (brightnessFilters[1], fabs(mid[i]));
    brightness[2] = next_sample (brightnessFilters[2], fabs(high[i]));
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
  glutKeyboardFunc (Keyboard);
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

  sr=jack_get_sample_rate (client);

  analog_cutoff = tan (PI * cutoff / sr);
  analog_cutoff2 = tan (PI * cutoff2 / sr);

  int mainOrder = 2;
	
  dccutter = create_iirfilter (2, TYPE_HIGH, tan (PI / sr), analog_cutoff2); //get rid of any dc
  avcfilter = create_iirfilter (2, TYPE_LOW, tan (PI * AVC_FREQ / sr), 0.0);
  lowfilter = create_iirfilter (mainOrder, TYPE_LOW, tan (PI * LOW_MID_FREQ / sr), 0.0);
  midfilter = create_iirfilter (mainOrder, TYPE_BAND, tan (PI * LOW_MID_FREQ / sr), tan (PI * MID_HIGH_FREQ / sr));
  highfilter = create_iirfilter (mainOrder, TYPE_HIGH, tan (PI * MID_HIGH_FREQ / sr), 0.0);
  for (int i = 0; i < 3; i++)
    {
      brightnessFilters [i] = create_iirfilter (4, TYPE_LOW, tan (PI * BRIGHTNESS_FREQ / sr), 0.0);
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

  glDeleteTextures (1, &texNum);

  destroy_iirfilter (dccutter);
  destroy_iirfilter (avcfilter);
  destroy_iirfilter (lowfilter);
  destroy_iirfilter (midfilter);
  destroy_iirfilter (highfilter);
  for (int i = 0; i < 3; i++)
    {
      destroy_iirfilter (brightnessFilters [i]);
    }
	
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
  gluPerspective (60.0f, fAspect, 10.0, 1000);
  
  //state should be MODELVIEW by default
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
}

// This function does any needed initialization on the rendering
// context.  Here it sets up and initializes the lighting for
// the scene.
void SetupScene()
{
  glEnable(GL_DEPTH_TEST);	// Hidden surface removal
  glFrontFace(GL_CW);		// Counter clock-wise polygons face out
  //glEnable(GL_CULL_FACE);		// Do not calculate inside of jet


  // Enable lighting
  glEnable(GL_LIGHTING);

  // Enable color tracking
  glEnable(GL_COLOR_MATERIAL);
	
  // Set Material properties to follow glColor values
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  
  // Black blue background
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  // set up "texture"
  for (int i = 0; i < 64; i++)
    {
      for (int j = 0; j < 64; j++)
	{
	  prgTex[i][3 * j + 0] = 4 * i;
	  prgTex[i][3 * j + 1] = 4 * abs (32 - i);
	  prgTex[i][3 * j + 2] = 4 * (64 - i);
	}
    }
  glGenTextures (1, &texNum);
  glBindTexture (GL_TEXTURE_2D, texNum);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D (GL_TEXTURE_2D, //target
		0, //level
		GL_RGB, //internal format
		64, 64, //width, height
		0, //border
		GL_RGB, //data format
		GL_UNSIGNED_BYTE, //unit of data
		prgTex //texture
		);
}

//
void Keyboard(unsigned char key, int x, int y){
  switch(key){
  case ' ':
    isRotating = !isRotating;
    startTick = clock ();
    break;
  case 'q':
    userBrightness[3] = fmin (userBScaleFactor * userBrightness[3], userBScaleMax);
    break;
  case 'a':
    userBrightness[3] /= userBScaleFactor;
    break;
  case 'w':
    userBrightness[0] = fmin (userBScaleFactor * userBrightness[0], userBScaleMax);
    break;
  case 's':
    userBrightness[0] /= userBScaleFactor;
    break;
  case 'e':
    userBrightness[1] = fmin (userBScaleFactor * userBrightness[1], userBScaleMax);
    break;
  case 'd':
    userBrightness[1] /= userBScaleFactor;
    break;
  case 'r':
    userBrightness[2] = fmin (userBScaleFactor * userBrightness[2], userBScaleMax);
    break;
  case 'f':
    userBrightness[2] /= userBScaleFactor;
    break;
  default:
    break;
  }
  return;
}

// Respond to arrow keys
void SpecialKeys(int key, int x, int y)
{
  if(key == GLUT_KEY_UP)
    xRot-= 5.0f;

  if(key == GLUT_KEY_DOWN)
    xRot += 5.0f;

  if (!isRotating)
    {
      if(key == GLUT_KEY_LEFT)
	yRot -= 5.0f;

      if(key == GLUT_KEY_RIGHT)
	yRot += 5.0f;
    }
                
  xRot = (GLfloat)((const int)xRot % 360);
  yRot = (GLfloat)((const int)yRot % 360);

  // Refresh the Window
  glutPostRedisplay();
}

void MultiQuad (GLfloat width, GLfloat height, int wNum, int hNum, int posNormal)
{
  glNormal3f (0.f, 0.f, posNormal ? 1 : -1);
    if (wNum < 1)
    { wNum = 1;
    }
  if (hNum < 1)
    { hNum = 1;
    }
  float tileWidth = width / wNum;
  float tileHeight = height / hNum;
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texNum);

  glBegin (GL_QUADS);
  for (int i = 0; i < wNum; i++)
    {
      for (int j = 0; j < hNum; j++)
	{
	  glTexCoord2d (i/(float)wNum, j/(float)hNum); glVertex3f (i * tileWidth, j * tileHeight, 0.0f);
	  glTexCoord2d (i/(float)wNum, (j+1)/(float)hNum); glVertex3f (i * tileWidth, (j + 1) * tileHeight, 0.0f);
	  glTexCoord2d ((i+1)/(float)wNum, (j+1)/(float)hNum); glVertex3f ((i + 1) * tileWidth, (j + 1) * tileHeight, 0.0f);
	  glTexCoord2d ((i+1)/(float)wNum, j/(float)hNum); glVertex3f ((i + 1) * tileWidth, j * tileHeight, 0.0f);
	}
    }
  glEnd ();
  glDisable (GL_TEXTURE_2D);
}

void MultiColumn (double width, double height, int wNum, int hNum, int top, int bottom, int posNormal)
{ double xTrans[4] = {-.5, .5, .5, -.5};
  double zTrans[4] = {.5, .5, -.5, -.5};
  
  for (int i = 0; i < 4; i++)
    {
      glPushMatrix ();
      glTranslatef (width * xTrans[i], 0.0, width * zTrans[i]);
      glRotatef (90 * i, 0., 1., 0.);
      MultiQuad (width, height, wNum, hNum, posNormal);
      glPopMatrix ();
    }
  if (top)
    {
      glPushMatrix ();
      glTranslatef (-.5 * width, height, .5 * width);
      glRotatef (-90, 1., 0., 0.);
      MultiQuad (width, width, wNum, wNum, posNormal);
      glPopMatrix ();
    }
    if (bottom)
    {
      glPushMatrix ();
      glTranslatef (-.5 * width, 0, -.5 * width);
      glRotatef (90, 1., 0., 0.);
      MultiQuad (width, width, wNum, wNum, posNormal);
      glPopMatrix ();
    }
}
	 

// Called to draw scene
void RenderScene(void)
{
  // Clear the window with current clearing color
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
  float wallWidth = 500;
  float columnWidth = 20;
  float wallHeight = 200;
  float barScale = 500;
  float avcScale = 1;
  float barFZ = columnWidth / 2;
  float barBZ = barFZ  - columnWidth;
  float barWidth = 20;
  float barSpacing = 90;
  float topLightScale = .9;
  float wallBrightness = .2;
  float lightHeightThreshold = .5;
  float rotFreq = 5;
  float pieSpacingFactor = .3;
  float pieRadius = 30;

  // Save the matrix state and do the rotations
  glPushMatrix();
  glTranslatef(0.0f, -.5 * wallHeight, -.45 * wallWidth);
  glRotatef(xRot, 1.0f, 0.0f, 0.0f);
  if (isRotating)
    yRot = (int) ((clock () - startTick) / ((float)CLOCKS_PER_SEC) * (360. / rotFreq)) % 360;    
  glRotatef(yRot, 0.0f, 1.0f, 0.0f);

  //room space, at 0 altitude, in the middle of the floor

  // set up after camera so that they are fixed into the scene
  //just a small gray so that we can see everything
  //  GLfloat whiteLight[] = { 0.1f, 0.1f, 0.1f, 1.0f };
  //glLightModelfv(GL_LIGHT_MODEL_AMBIENT, whiteLight);
  //set up these arrays for convenience
  GLfloat lightPos[] = {0., 0., 0., 1.0f};
  GLfloat sourceLight [] = {0., 0., 0., 1.0f};

  glColor3f (wallBrightness, wallBrightness, wallBrightness);
  //front wall
  glPushMatrix ();
  glTranslatef (-.5 * wallWidth, 0.0f, -.5 * wallWidth);
  MultiQuad (wallWidth, wallHeight, 10, 10, GL_TRUE);
  glPopMatrix ();
  //left wall
  glPushMatrix ();
  glTranslatef (-.5 * wallWidth, 0., .5 * wallWidth);
  glRotatef (90., 0., 1.0, 0.);
  MultiQuad (wallWidth, wallHeight, 10, 10, GL_TRUE);
  glPopMatrix ();
  //right wall
  glPushMatrix ();
  glTranslatef (.5 * wallWidth, 0., -.5 * wallWidth);
  glRotatef (-90., 0., 1.0, 0.);
  MultiQuad (wallWidth, wallHeight, 10, 10, GL_TRUE);
  glPopMatrix ();
  //rear wall
  glPushMatrix ();
  glTranslatef (.5 * wallWidth, 0., .5 * wallWidth);
  glRotatef (-180., 0., 1.0, 0.);
  MultiQuad (wallWidth, wallHeight, 10, 10, GL_TRUE);
  glPopMatrix ();
  //floor
  glPushMatrix ();
  glTranslatef (-.5 * wallWidth, 0.0f, .5 * wallWidth);
  glRotatef (-90.0, 1.0, 0.0, 0.);
  MultiQuad (wallWidth, wallWidth, 10, 10, GL_TRUE);
  glPopMatrix ();
 
  float lightFactor = 1;
  //left
  glColor3f (1., 0., 0.);
  float leftHeight = fmin (barScale * (brightness[0] * userBrightness[0] * userBrightness[3]
				       / (1 + avcScale * avc)), wallHeight);
  glPushMatrix ();
  glTranslatef (-barSpacing, 0., 0.);
  MultiColumn (barWidth, leftHeight, 10, 10, GL_TRUE, GL_FALSE, GL_FALSE);
  lightFactor = fmin (1.0, leftHeight / (lightHeightThreshold * wallHeight));
  sourceLight [0] = lightFactor * 1.;
  lightPos [1] = lightFactor * topLightScale * leftHeight;
  glLightfv(GL_LIGHT0, GL_DIFFUSE, sourceLight);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
  glEnable(GL_LIGHT0);
  sourceLight [0] = 0.;
  glPopMatrix ();
  //mid
  glColor3f (0.0f, 1.0f, 0.0f);
  float midHeight = fmin (barScale * (brightness [1] * userBrightness[1] * userBrightness[3]
				      / (1 + avcScale * avc)), wallHeight);
  MultiColumn (barWidth, midHeight, 10, 10, GL_TRUE, GL_FALSE, GL_FALSE);
  lightFactor = fmin (1.0, midHeight / (lightHeightThreshold * wallHeight));
  sourceLight [1] = lightFactor * 1.;
  lightPos [1] = lightFactor * topLightScale * midHeight;
  glLightfv(GL_LIGHT1, GL_DIFFUSE, sourceLight);
  glLightfv(GL_LIGHT1, GL_POSITION, lightPos);
  glEnable(GL_LIGHT1);
  sourceLight [1] = 0.;
  //right
  glColor3f (0.0f, 0.0f, 1.0f);
  float rightHeight = fmin (barScale * (brightness [2] * userBrightness[2] * userBrightness[3]
					/ (1 + avcScale * avc)), wallHeight);
  glPushMatrix ();
  glTranslatef (barSpacing, 0., 0.);
  MultiColumn (barWidth, rightHeight, 10, 10, GL_TRUE, GL_FALSE, GL_FALSE);
  lightFactor = fmin (1.0, rightHeight / (lightHeightThreshold * wallHeight));
  sourceLight [2] = lightFactor * 1.;
  lightPos [1] = lightFactor * topLightScale * rightHeight;
  glLightfv(GL_LIGHT2, GL_DIFFUSE, sourceLight);
  glLightfv(GL_LIGHT2, GL_POSITION, lightPos);
  glEnable(GL_LIGHT2);
  sourceLight [2] = 0.;
  glPopMatrix ();

  glDisable (GL_LIGHTING);
  glPushMatrix ();
  glTranslatef (0, .5 * wallHeight, .45 * wallWidth);
  float pieStartX = -1 * pieSpacingFactor * wallWidth;
  float pieSpace = pieSpacingFactor * wallWidth;
  for (int i = 0; i < 3; i++)
    {
      glBegin (GL_TRIANGLE_FAN);
      glColor3f (1., 1., 1.);
      glVertex3f (pieStartX + i * pieSpace, 0, 0);
      for (float theta = 0; theta < 2 * PI * userBrightness[i] * userBrightness[3] / userBScaleMax;
	   theta += 2 * PI / 100)
	{
	  glVertex3f (pieStartX + i * pieSpace + pieRadius * cos (theta),
		    pieRadius * sin (theta), 0);
	}
      glEnd ();
    }
  glPopMatrix ();
  glEnable (GL_LIGHTING);
  
  // Restore the matrix state
  glPopMatrix();

  // Buffer swap
  glutSwapBuffers();
}

