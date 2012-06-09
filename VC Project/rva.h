#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>

#include <iostream>
#include <vector>
#include <ctime>

#include "glm.h"

#define MAX_PATTS 2
#define FLOW_TIME 2.0
#define MAX_OBJS  4
// Max distance to consider a marker-teapot intersection
#define THRESHOLD 70.0

// projection to model coordinate transformation parameters
#define CAMSIZE_X 280.0
#define CAMSIZE_Y 250.0
#define SCENESIZE_X 4.0
#define SCENESIZE_Y 4.0

static void init(void);
static void cleanup(void);
static void keyEvent( unsigned char key, int x, int y);
static void loop(void);
static void draw(double trans[3][4], unsigned i);
static void draw_objs(void);
static void initPatts(void);
static void handleDet(ARMarkerInfo m, int pid);
static void initObjs(void);
static void handle_objs(double trans[3][4]);

using namespace std;

// Camera configuration file
#ifdef _WIN32
//char			vconf[] = "WDM_camera_flipV.xml";
char			*vconf = NULL;
#else
char			*vconf = "";
#endif

// Global variables
int      xsize, ysize;
int      thresh = 100;	//< marker detection threshold
int      count = 0;		//< frame counter
char     *cparam_name    = "camera_para.dat";	//< camera settings file
ARParam  cparam;
char     *patt_name[MAX_PATTS];	//< pattern names to load
int      patt_id[MAX_PATTS];	//< loaded pattern ids
// Pattern configurations
int      patt_width     = 80.0;
double   patt_center[2] = {0.0, 0.0};
double   patt_trans[3][4];
// Teapots position and draw flag
float	 obj_pos[MAX_OBJS][2];
bool	 obj_draw[MAX_OBJS];

// Object colors 
#define EMBALAGEM		0
#define PAPEL_CARTAO	1
#define VIDRO			2
#define INDIFERENCIADO	3
#define PILHAO			4

char patt_color[MAX_PATTS];
char obj_color[MAX_OBJS];
GLMmodel * obj_model[MAX_OBJS];

// To control score
#define UP				500
#define DOWN			100
int score = 0;

// Class of each trail effect position
class EffectCoords{
public:
	double pattTrans[3][4];
	double time;
	void setPattTrans(double[3][4]);
};

// Vector with all the positions of all trail effects
vector<EffectCoords> effcoords[MAX_PATTS];
