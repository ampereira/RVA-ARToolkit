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

using namespace std;

#define MAX_PATTS 2
#define FLOW_TIME 2.0
#define MAX_OBJS  1
#define THRESHOLD 10.0

/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

int             xsize, ysize;
int             thresh = 100;
int             count = 0;

int             mode = 1;

char           *cparam_name    = "camera_para.dat";
ARParam         cparam;

char           *patt_name[MAX_PATTS];
int             patt_id[MAX_PATTS];
int             patt_width     = 80.0;
double          patt_center[2] = {0.0, 0.0};
double          patt_trans[3][4];
float			obj_pos[MAX_OBJS][2];

class EffectCoords{
public:
	double pattTrans[3][4];
	double time;
	void setPattTrans(double[3][4]);
};

void EffectCoords::setPattTrans(double pt[3][4]){
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 4; ++j)
			pattTrans[i][j] = pt[i][j];
}

vector<EffectCoords> effcoords[MAX_PATTS];

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   loop(void);
void   draw( double trans[3][4], unsigned i );
void   draw2();



int main(int argc, char **argv)
{
	glutInit(&argc, argv);
    init();

    arVideoCapStart();
    argMainLoop( NULL, keyEvent, loop );
	return (0);
}

static void initPatts(){

	for(unsigned i = 0; i < MAX_PATTS; ++i){
		switch(i){
			case 0 : patt_name[0] = "patt.hiro"; break;
			case 1 : patt_name[1] = "patt.kanji"; break;
			case 2 : patt_name[2] = "patt.sample1"; break;
			case 3 : patt_name[3] = "patt.sample2"; break;

		}
		if( (patt_id[i]=arLoadPatt(patt_name[i])) < 0 ) {
			cout << "Error loading " << patt_name[i] << " pattern!" << endl;
			exit(0);
		}
	}
}

static void   keyEvent( unsigned char key, int x, int y)
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }

    if( key == 'c' ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;

        mode = 1 - mode;
        if( mode ) printf("Continuous mode: Using arGetTransMatCont.\n");
         else      printf("One shot mode: Using arGetTransMat.\n");
    }
}

void handleDet(ARMarkerInfo m, int pid){

	arGetTransMat(&m, patt_center, patt_width, patt_trans);
	EffectCoords ef;
	ef.time =  arUtilTimer();
	ef.setPattTrans(patt_trans);
	effcoords[pid].push_back(ef);

}

static void loop(void){
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int              marker_num;
    int              j, i;
	bool flag;

    // try to grab a video frame and if it can't function exits
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }

    argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        arVideoCapStop();
		arVideoClose();
		argCleanup();
        exit(0);
    }

    arVideoCapNext();

    // checks if a pattern is visible and sends the information to handleDet
    for( j = 0; j < marker_num; j++ ) {
		for(i = 0; i < MAX_PATTS; ++i){
			if(patt_id[i] == marker_info[j].id)
				handleDet(marker_info[j], i);
		}
    }
	
	draw2();
	for(unsigned k = 0; k < MAX_PATTS; ++k){
		flag = true;
		for(unsigned l = 0; (l < effcoords[k].size()) && flag; ++l)
			if((arUtilTimer() - effcoords[k][l].time) > FLOW_TIME)
				effcoords[k].erase(effcoords[k].begin() + l);
			else	
				draw(effcoords[k][l].pattTrans, k);
	}

    argSwapBuffers();
}

static void initObjs(void){
	obj_pos[0][0] = 0.0;
	obj_pos[0][1] = 0.0;
}

static void init( void )
{
    ARParam  wparam;

    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoInqSize(&xsize, &ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
        printf("Camera parameter load error !!\n");
        exit(0);
    }
    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

	initPatts();
	initObjs();

    /* open the graphics window */
    argInit( &cparam, 1.0, 0, 0, 0, 0 );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

void draw( double trans[3][4], unsigned i )
{
    double    gl_para[16];
    GLfloat   mat_ambient[]     = {0.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash[]       = {0.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    argConvGlpara(trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd( gl_para );

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMatrixMode(GL_MODELVIEW);
    glTranslatef( 0.0, 0.0, 25.0 );
	
	switch(i){
		case 0 : mat_ambient[2] = 1.0;
				 mat_flash[2]   = 1.0;
				 break;
		case 1 : mat_ambient[1] = 1.0;
				 mat_flash[1]   = 1.0;
				 break;
		case 2 : mat_ambient[0] = 1.0;
				 mat_flash[0]   = 1.0;
				 break;
		case 3 : mat_ambient[0] = 0.2;
				 mat_flash[0]   = 0.2;
				 break;
	}

	glutSolidSphere(5.0, 10, 10);

	float aux = sqrt(trans[0][3]*trans[0][3] + trans[1][3]*trans[1][3]) -
				sqrt(obj_pos[0][0]*obj_pos[0][0] + obj_pos[0][1]*obj_pos[0][1]);

	if(aux < THRESHOLD){
		cout << "JA FOSTE" << endl;
		Sleep(3000);
		exit(0);
	}
	glDisable( GL_LIGHTING );

    glDisable( GL_DEPTH_TEST );
}
// supostamente desenha a cena estatica
// nao consigo acertar com as coords para se ver o teapot
void draw2()
{
    double    gl_para[16];
    GLfloat   mat_ambient[]     = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash[]       = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    
  float aspect = 1.0;
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
   // glDepthFunc(GL_LEQUAL); 
	glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    
    /* load the camera transformation matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45.0,aspect ,1.0,100);
	//glMatrixMode(GL_MODELVIEW);
  
	glPushMatrix();
	glTranslatef(obj_pos[0][0], obj_pos[0][1], -10.0);
	glutSolidTeapot(0.5);
	glPopMatrix();

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_LIGHTING );
}
