#include "rva.h"

void EffectCoords::setPattTrans(double pt[3][4]){
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 4; ++j)
			pattTrans[i][j] = pt[i][j];
}


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
    init();

    arVideoCapStart();
    argMainLoop( NULL, keyEvent, loop );
	return (0);
}

// Loads the patterns to be recognized
static void initPatts(void){

	for(unsigned i = 0; i < MAX_PATTS; ++i){
		switch(i){
			case 0 : 
				patt_name[0] = "patt.hiro";
				patt_color[0]= EMBALAGEM;
				break;
			case 1 : 
				patt_name[1] = "patt.kanji";
				patt_color[1]= PAPEL_CARTAO;
				break;
			case 2 : 
				patt_name[2] = "patt.sample1";
				patt_color[2]= VIDRO;
				break;
			case 3 : 
				patt_name[3] = "patt.sample2";
				patt_color[3]= INDIFERENCIADO;
				break;

		}
		if( (patt_id[i]=arLoadPatt(patt_name[i])) < 0 ) {
			cout << "Error loading " << patt_name[i] << " pattern!" << endl;
			exit(0);
		}
	}
}

// Handles key pressing...
static void keyEvent( unsigned char key, int x, int y){
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
		Sleep(500);
        cleanup();
        exit(0);
    }
}

// Handle marker detection
static void handleDet(ARMarkerInfo m, int pid){
	// Adds the position to the marker trail vector
	arGetTransMat(&m, patt_center, patt_width, patt_trans);
	EffectCoords ef;
	// Adds time of the position to later check if it's older
	// than the defined trail length
	ef.time =  arUtilTimer();
	ef.setPattTrans(patt_trans);
	effcoords[pid].push_back(ef);

}

// Handle the teapot and marker collisions
static void handle_objs(double trans[3][4], unsigned patt_id){
	for(unsigned j = 0; j < MAX_OBJS; ++j){
		if(obj_draw[j]==false)
			continue;

		// Transform the teapots coords from model view to the camera coords
		float x = CAMSIZE_X * obj_pos[j][0] / SCENESIZE_X;
		// y nos objs ta invertido nao sei porque... (sistemas de coordenadas?)
		float y = - CAMSIZE_Y * obj_pos[j][1] / SCENESIZE_Y;
		float xx = trans[0][3] - x;
		float yy = trans[1][3] - y;

		// Distance between points, by measuring the magnitude of the vector between them
		float aux = sqrt(xx*xx + yy*yy);
		
		// If the pattern intersects a teapot
		if(abs(aux) < THRESHOLD){
			obj_draw[j] = false;

			if(obj_color[j]==patt_color[patt_id])	{
				score+=UP;
				switch(rand()%3)	{
				case 0:
				case 1:
					cout << "BOA, ";
					break;
				case 2:
					cout << "FIXE, ";
				}
				cout << " o teu score agora e " << score << endl;
			}
			else	{
				score-=DOWN;
				cout << "\aContentor errado, o teu score agora e " << score << endl;
			}

			// If all teapots were intersected game over!
			bool not_end=false;
			for(unsigned i=0 ; i<MAX_OBJS ; ++i)	{
				not_end = not_end || obj_draw[i];
			}
			if(!not_end)	{
				
				if(score>0)
					cout << "You WON!" << endl;
				else
					cout << "You LOSE!" << endl;
				cout << "Your final score is " << score << "." << endl;
				getchar();
				cleanup();
				exit(0);
			}
		}
	}
}

// Loop called every frame
static void loop(void){
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int              marker_num;
    int              j, i;

    // Try to grab a video frame and if it can't function exits
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }

    argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

    // Detect the markers in the video frame 
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
        exit(0);
    }

    arVideoCapNext();

    // Checks if a pattern is visible and sends the information to handleDet
    for( j = 0; j < marker_num; j++ ) {
		for(i = 0; i < MAX_PATTS; ++i){
			if(patt_id[i] == marker_info[j].id)
				handleDet(marker_info[j], i);
		}
    }
	
	// Draw the teapots
	draw_objs();

	// Draws the 2 seconds trail of the markers
	for(unsigned k = 0; k < MAX_PATTS; ++k)	{
		for(unsigned l = 0; l < effcoords[k].size(); ++l)	{
			// Removes spheres older than 2 seconds
			if((arUtilTimer() - effcoords[k][l].time) > FLOW_TIME)
				effcoords[k].erase(effcoords[k].begin() + l);
			else{	
				// Handles teapots colisions and draws the trail
				handle_objs(effcoords[k][l].pattTrans, k);
				draw(effcoords[k][l].pattTrans, k);
			}
		}
	}

    argSwapBuffers();
}

// Initializes the start positions and colors of the teapots
static void initObjs(void){
	obj_pos[0][0] = -2.3;
	obj_pos[0][1] = 3.0;
	
	obj_pos[1][0] = 3.0;
	obj_pos[1][1] = 2.3;
	
	obj_pos[2][0] = -3.0;
	obj_pos[2][1] = -2.3;
	
	obj_pos[3][0] = 2.3;
	obj_pos[3][1] = -3.0;

	for(unsigned i = 0; i < MAX_OBJS; ++i)	{
		obj_draw[i]		= true;
		obj_color[i]	= rand()%4;
		char buffer[1024];
		char file[128];
		switch(obj_color[i])	{
			case EMBALAGEM:
				sprintf(file,"Jug1.obj");
				break;
			case PAPEL_CARTAO:
				sprintf(file,"book.obj");
				break;
			case VIDRO:
				sprintf(file,"cups.obj");
				break;
			case INDIFERENCIADO:
				sprintf(file,"indiferenciado.obj");
				break;
		}
		
		if(obj_color[i]==INDIFERENCIADO ) //||obj_color[i]==VIDRO
			continue;

		sprintf(buffer,"C:\/cygwin\/home\/Rafael\/CG\/RVA\/ARToolkit\/%s",file);
		obj_model[i]	= glmReadOBJ(buffer);
		glmUnitize(obj_model[i]);
		glmFacetNormals(obj_model[i]); 
		glmVertexNormals(obj_model[i], 90.0);
	}
}

static void init(void){
    ARParam  wparam;

    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(1);
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

	// Initializes the patterns and teapots
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

// Draws a sphere in the position trans, based on the pattern index i
static void draw( double trans[3][4], unsigned i ){

    double    gl_para[16];
    GLfloat   mat_ambient[]     = {0.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash[]       = {0.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    
	// Choses the trail color for each marker
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

	// Lighting and color settings...
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

	glutSolidSphere(5.0, 10, 10);

	glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
}

// Draws the teapots
static void draw_objs(void){

    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    
	float aspect = xsize / ysize;

    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); 


	glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    
    // Draws on the projection space
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45.0, aspect, 1.0, 100);
  
	// 4000 milisecond movement...
	int x = (int)(clock()/(CLOCKS_PER_SEC/1e3)) % (int)4000;

	for(unsigned i = 0; i < MAX_OBJS; ++i){
		if(obj_draw[i]){
			// update obj position

			// angulo direcao
			double STEP = 0.1;
			static double ang[MAX_OBJS] = {
				(((float)rand())/RAND_MAX)*2*PI,
				(((float)rand())/RAND_MAX)*2*PI,
				(((float)rand())/RAND_MAX)*2*PI,
				(((float)rand())/RAND_MAX)*2*PI
			};

			// update position
			obj_pos[i][0] += STEP * sin(ang[i]);
			obj_pos[i][1] += STEP * cos(ang[i]);

			// handle colisions with the wall's
			if(obj_pos[i][0]>=3)
				ang[i] = (((float)rand())/RAND_MAX)*PI+PI/2;
			if(obj_pos[i][1]>=3)
				ang[i] = (((float)rand())/RAND_MAX)*PI+PI;
			if(obj_pos[i][0]<=-3)
				ang[i] = (((float)rand())/RAND_MAX)*PI-PI/2; 
			if(obj_pos[i][1]<=-3)
				ang[i] = (((float)rand())/RAND_MAX)*PI;
			//if(obj_pos[i][0]>=3 || obj_pos[i][1]>=3 || obj_pos[i][0]<=-3 || obj_pos[i][1]<=-3) // failed atempt do redirect to the center
				//ang[i] = atan(obj_pos[i][1]/obj_pos[i][0]);//+((((float)rand())/RAND_MAX)-0.5)*(PI/3);
			
			// Se sair da zona do ecra o obj e teleportado para o centro
			if(	(obj_pos[i][0]>=3	&& obj_pos[i][1]>=3) || //Canto superior direiro
				(obj_pos[i][0]<=-3	&& obj_pos[i][1]>=3) || //Canto superior esquerdo
				(obj_pos[i][0]>=3	&& obj_pos[i][1]<=-3) || //Canto inferior direiro
				(obj_pos[i][0]<=-3	&& obj_pos[i][1]<=-3))	{ //Canto inferior esquerdo
					obj_pos[i][0]=0;
					obj_pos[i][1]=0;
			}

			if(obj_color[i]==EMBALAGEM) // for debug
				cout << "obj[i].XX = " << obj_pos[i][0] << "\t |\tobj[i].YY = " << obj_pos[i][1] << endl;

			GLfloat   mat_ambient[]     = {0.0, 0.0, 0.0, 1.0};
			GLfloat   mat_flash[]       = {0.0, 0.0, 0.0, 1.0};
			GLfloat   mat_flash_shiny[] = {50.0};
			
			switch(obj_color[i])		{
				case EMBALAGEM:
					mat_ambient[0]	=1.0;
					mat_flash[0]	=1.0;
					mat_ambient[1]	=1.0;
					mat_flash[1]	=1.0;
					break;
				case PAPEL_CARTAO:
					mat_ambient[2]	=1.0;
					mat_flash[2]	=1.0;
					break;
				case VIDRO:
					mat_ambient[1]	=1.0;
					mat_flash[1]	=1.0;
					break;
				case PILHAO:
					mat_ambient[0]	=1.0;
					mat_flash[0]	=1.0;
					break;
				case INDIFERENCIADO:
					mat_ambient[0]	=0.745;
					mat_flash[0]	=0.745;
					mat_ambient[1]	=0.745;
					mat_flash[1]	=0.745;
					mat_ambient[2]	=0.745;
					mat_flash[2]	=0.745;
			}
			glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
			glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
			glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);

			glPushMatrix();
			glTranslatef(obj_pos[i][0], obj_pos[i][1], -10.0);

			// Rotation of the teapots to give a nicer look!
			glRotatef((float)clock()/(CLOCKS_PER_SEC/1e2), abs(cos((float)clock()/(CLOCKS_PER_SEC/1e2))), 
				abs(cos((float)clock()/(CLOCKS_PER_SEC/1e2))), 0);
			
			if(obj_model[i]==NULL)
				glutSolidTeapot(0.5);
			else	{
				//switch(obj_color[i])		{
				//case EMBALAGEM:
				//	cout << "EMBALAGEM" << endl;
				//	break;
				//case PAPEL_CARTAO:
				//	cout << "PAPEL_CARTAO" << endl;
				//	break;
				//case VIDRO:
				//	cout << "VIDRO" << endl;
				//	break;
				//case PILHAO:
				//	cout << "PILHAO" << endl;
				//	break;
				//case INDIFERENCIADO:
				//	cout << "INDIFERENCIADO" << endl;
				//}
				if(obj_color[i]!=VIDRO)	{
					glmDraw(obj_model[i],GLM_SMOOTH|GLM_MATERIAL|GLM_TEXTURE);
				} else
					glutSolidTeapot(0.5);
			}

			glPopMatrix();
		}
	}

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_LIGHTING );
}
