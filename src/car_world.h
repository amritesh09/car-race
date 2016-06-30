#include <GL/glut.h>    // Header File For The GLUT Library 
#include <GL/gl.h>	// Header File For The OpenGL32 Library
#include <GL/glu.h>	// Header File For The GLu32 Library
#include <unistd.h>     // Header file for sleeping.
#include <stdio.h>      // Header file for standard file i/o.
#include <stdlib.h>     // Header file for malloc/free.
#include <math.h>       // Header file for trigonometric functions.
#include <string.h>
#include "png.c"
#include "obj.h"
/* the ascii codes for various special keys */
#define ESCAPE 27
#define SPACE 32
#define PAGE_UP 73
#define PAGE_DOWN 81
#define UP_ARROW 72
#define DOWN_ARROW 80
#define LEFT_ARROW 75
#define RIGHT_ARROW 77
#define NUM 7				//1+ai cars

int window; 
int menu=1;
GLuint loop,straight,left;             
GLuint texture[3],texture_bg,texture_car;       

int keybuff[246];
int light = 0;           
int blend = 0;        
GLfloat drift=1.0f;
GLfloat xrot;            
GLfloat yrot;            
GLfloat xspeed;          
GLfloat yspeed,tmp[50];         
GLfloat run=0.0f;GLfloat airun[50]={0.0f,0.0f};GLfloat airunx[50]={0.0f,0.0f};GLfloat airunx2[50]={0.0f,0.0f};
GLfloat walkbias = 0;
GLfloat walkbiasangle = 0;
GLfloat angle[50]={0.0f,0.0f};
GLfloat lookupdown = 0.0;		//0.0
GLfloat speed=0.2;
const float piover180 = 0.0174532925f;

float heading, xpos, zpos;

GLfloat camx = 0, camy = 0, camz = 0; // camera location.
GLfloat therotate,carx,carz;

GLfloat z=0.0f;                      

GLfloat LightAmbient[]  = {0.5f, 0.5f, 0.5f, 1.0f}; 
GLfloat LightDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f}; 
GLfloat LightPosition[] = {0.0f, 0.0f, 2.0f, 1.0f};

GLuint filter = 1;       // texture filtering method to use (nearest, linear, linear + mipmaps)

typedef struct {         // vertex coordinates - 3d and texture
    GLfloat x, y, z;     // 3d coords.
    GLfloat u, v;        // texture coords.
} VERTEX;

typedef struct {         // triangle
    VERTEX vertex[4];    // 3 vertices array
} TRIANGLE;

typedef struct {         // sector of a 3d environment
    int numtriangles;    // number of triangles in the sector
    TRIANGLE* triangle;  // pointer to array of triangles.
} SECTOR;

SECTOR sector1;

typedef struct{
	GLfloat x;
	GLfloat z;
	}location;
int lindex=0;
location loc[100];
/* Image type - contains height, width, and data */
typedef struct {
    unsigned long sizeX;
    unsigned long sizeY;
    char *data;
} Image;

// degrees to radians...2 PI radians = 360 degrees
float rad(float angle)
{
    return angle * piover180;
}

// helper for SetupWorld. 
void readstr(FILE *f, char *string)
{
    do {
	fgets(string, 255, f); // read the line
    } while ((string[0] == '/') || (string[0] == '\n'));
    return;
}

// loads the world from a text file.
void SetupWorld() 
{	int i=0;
	for(i=0;i<246;i++)keybuff[i]=0;
    float x, y, z, u, v;
    int vert;
    int numtriangles;
    FILE *filein;        // file to load the world from
    char oneline[255];

    filein = fopen("resources/world8.txt", "rt");

    readstr(filein, oneline);
    sscanf(oneline, "NUMPOLLIES %d\n", &numtriangles);

    sector1.numtriangles = numtriangles;
    sector1.triangle = (TRIANGLE *) malloc(sizeof(TRIANGLE)*numtriangles);
    
    for (loop = 0; loop < numtriangles; loop++) {
	for (vert = 0; vert < 4; vert++) {
	    readstr(filein,oneline);
	    sscanf(oneline, "%f %f %f %f %f", &x, &y, &z, &u, &v);
	    sector1.triangle[loop].vertex[vert].x = x;
	    sector1.triangle[loop].vertex[vert].y = y;
	    sector1.triangle[loop].vertex[vert].z = z;
	    sector1.triangle[loop].vertex[vert].u = u;
	    sector1.triangle[loop].vertex[vert].v = v;
	}
    }

    fclose(filein);
    return;
}
    
/* getint and getshort are help functions to load the bitmap byte by byte*/
static unsigned int getint(fp)
     FILE *fp;
{
  int c, c1, c2, c3;

  // get 4 bytes
  c = getc(fp);  
  c1 = getc(fp);  
  c2 = getc(fp);  
  c3 = getc(fp);
  
  return ((unsigned int) c) +   
    (((unsigned int) c1) << 8) + 
    (((unsigned int) c2) << 16) +
    (((unsigned int) c3) << 24);
}

static unsigned int getshort(fp)
     FILE *fp;
{
  int c, c1;
  
  //get 2 bytes
  c = getc(fp);  
  c1 = getc(fp);

  return ((unsigned int) c) + (((unsigned int) c1) << 8);
}

int ImageLoad(char *filename, Image *image) 
{
    FILE *file;
    unsigned long size;                 // size of the image in bytes.
    unsigned long i;                    // standard counter.
    unsigned short int planes;          // number of planes in image
    unsigned short int bpp;             // number of bits per pixel
    char temp;                          // used to convert bgr to rgb color.

    // make sure the file is there.
    if ((file = fopen(filename, "rb"))==NULL) {
      printf("File Not Found : %s\n",filename);
      return 0;
    }
    
    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

     // read the width
    image->sizeX = getint (file);
    
    // read the height 
    image->sizeY = getint (file);
    
    // calculate the size 
    size = image->sizeX * image->sizeY * 3;

    // read the planes
    planes = getshort(file);
    if (planes != 1) {
	printf("Planes from %s is not 1: %u\n", filename, planes);
	return 0;
    }

    
    bpp = getshort(file);
    if (bpp != 24) {
      printf("Bpp from %s is not 24: %u\n", filename, bpp);
      return 0;
    }
	
    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    // read the data. 
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
	printf("Error allocating memory for color-corrected image data");
	return 0;	
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
	printf("Error reading image data from %s.\n", filename);
	return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
	temp = image->data[i];
	image->data[i] = image->data[i+2];
	image->data[i+2] = temp;
    }

     return 1;
}
struct obj_model_t mdl;
// Load Bitmaps And Convert To Textures
GLvoid LoadGLTextures(GLvoid) 
{	
    // Load Texture
    Image *image1;
    Image *image_bg;//bg image
    Image *image_car;
    // allocate space for texture
    image1 = (Image *) malloc(sizeof(Image));image_bg=(Image *) malloc(sizeof(Image));image_car=(Image *) malloc(sizeof(Image));
    if (image1 == NULL) {
	printf("Error allocating space for image");
	exit(0);
    }

    if (!ImageLoad("resources/road4.bmp", image1)) {
	exit(1);
    }        
   if (!ImageLoad("resources/bg1.bmp", image_bg)) {
	exit(1);
    }  
   
    // Create Textures	
    glGenTextures(3, &texture[0]);glGenTextures(1, &texture_bg);glGenTextures(1, &texture_car);
    

    // nearest filtered texture
    glBindTexture(GL_TEXTURE_2D, texture[0]);   // 2d texture (x and y size)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); // scale  when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); // scale  when image smalled than texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

	
    // linear filtered texture
    glBindTexture(GL_TEXTURE_2D, texture[1]);   // 2d texture (x and y size)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

	//texture[1] = loadPNGTexture ("resources/road2.png");
    // linear filtered texture  for background
    glBindTexture(GL_TEXTURE_2D, texture_bg);   // 2d texture (x and y size)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image_bg->sizeX, image_bg->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image_bg->data);

	
   /* // linear filtered texture  for car
    glBindTexture(GL_TEXTURE_2D, texture_car);   // 2d texture (x and y size)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image_car->sizeX, image_car->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image_car->data);*/
    
	//LOAD TEXTURE USING LIBPNG
	texture_car = loadPNGTexture ("resources/car.png");

	// mipmapped texture
    glBindTexture(GL_TEXTURE_2D, texture[2]);   // 2d texture (x and y size)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST); // scale mipmap when image smalled than texture
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image1->sizeX, image1->sizeY, GL_RGB, GL_UNSIGNED_BYTE, image1->data);
};

/* A general OpenGL initialization function.  Sets all of the initial parameters. */
GLvoid InitGL(GLsizei Width, GLsizei Height)	// We call this right after our OpenGL window is created.
{
    LoadGLTextures();                           // load the textures.
    glEnable(GL_TEXTURE_2D);                    // Enable texture mapping.
	straight=1;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);          // Set the blending function for translucency (note off at init time)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// This Will Clear The Background Color To Black
    glClearDepth(1.0);				// Enables Clearing Of The Depth Buffer
    glDepthFunc(GL_LESS);                       // type of depth test to do.
    glEnable(GL_DEPTH_TEST);                    // enables depth testing.
    glShadeModel(GL_SMOOTH);			// Enables Smooth Color Shading
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();				// Reset The Projection Matrix
    
    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.5f,100.0f);	// Calculate The Aspect Ratio Of The Window
    
    glMatrixMode(GL_MODELVIEW);
	//lookupdown = 4.0;
    // set up lights.
    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
    glEnable(GL_LIGHT1);
}

/* The function called when our window is resized */
GLvoid ReSizeGLScene(GLsizei Width, GLsizei Height)
{lookupdown=8.0;
    if (Height==0)				// Prevent A Divide By Zero If The Window Is Too Small
	Height=1;
	//edit for view rectangle
    glViewport(0, 0, Width, Height);		// Reset The Current Viewport And Perspective Transformation

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.5f,200.0f);//100 initially
    glMatrixMode(GL_MODELVIEW);
}

void keyOperations (void) {
if(menu!=1){
 if(keybuff[GLUT_KEY_UP]==1){
	xpos -= (float)sin(yrot*piover180) * 0.5f;//printf("xpos=%f\n",xpos);
	zpos -= (float)cos(yrot*piover180) * speed;//printf("zpos=%f\n",zpos);//0.05f;//10.0f;//0.05f
	if(speed<=2.8)speed+=0.01f;	
	if (walkbiasangle >= 359.0f)
	    walkbiasangle = 0.0f;
//keep car in track
	if(xpos<-1.5f&&xpos>-2.0&&zpos>-292)xpos=-1.5f;	
	if(xpos>1.5f&&zpos>-292)xpos=1.5f;

	if(zpos>-294&&zpos<-293.0&&xpos<-3.0f&&xpos>-294.0f)zpos=-294.0f;
	if(zpos<-300&&xpos<-3.0f&&xpos>-294.0f)zpos=-300.0f;

 	if(xpos<-301.5f&&zpos>-292)xpos=-301.5f;	
	if(xpos>-298.5f&&xpos<-290.5f&&zpos>-292&&zpos<-9)xpos=-298.5f;

	if(zpos>-3&&zpos<0.5&&xpos<-3)zpos=-3.0f;
	if(zpos<-9&&zpos>-12&&xpos<-3)zpos=-9.0f;
		
	//else 
	   // walkbiasangle+= 10;		wave effect
	//walkbias = (float)sin(walkbiasangle * piover180)/20.0f;
	
	}

 if(keybuff[GLUT_KEY_DOWN]==1){ // walk back (bob head)
	xpos += (float)sin(yrot*piover180) * 0.05f;
	zpos += (float)cos(yrot*piover180) * 0.5f;//0.05f;//10.0f;	
	if (walkbiasangle <= 1.0f)
	    walkbiasangle = 359.0f;	
	else 
	    //walkbiasangle-= 10;
	walkbias = (float)sin(walkbiasangle * piover180)/20.0f;
	}
if(keybuff[GLUT_KEY_LEFT]==1){ // look left
	yrot += 0.5f*drift;drift+=0.1;//1.5f
	//if(speed>2.0f)speed-=0.001f;
	if(yrot>=90&&yrot<=180){straight=0;left=1;}
	}
    
    if(keybuff[GLUT_KEY_RIGHT]==1){ // look right
	yrot -= 0.5f;//1.5f
	//if(speed>2.0f)speed-=0.001f;
	}
}
}

void drawCar(GLfloat x,GLfloat y,GLfloat z,GLfloat angle){		//draw ai cars
	glNormal3f( 0.0f, 0.0f, 1.0f);//printf("called for %f  %f  %f\n",x,y,z);
	glBindTexture(GL_TEXTURE_2D, texture_car);
	glScalef(0.3,0.3,0.3);				//change size of car
	loc[lindex%NUM].x=x;loc[lindex%NUM].z=z;	//store current location for all ai cars
	glTranslatef (x, y, z);//change run to change speed   ytrans* changes perspective of car
	glRotatef(180.0f+angle, 0, 1.0f, 0);			//ROTATE ABOUT Y AND STRAIGHTEN THE CAR
	RenderOBJModel (&mdl);
	glFlush();glScalef(1/0.3,1/0.3,1/0.3);	
	lindex++;
	}
GLfloat mod(GLfloat x){if(x>0.0)return x;else return -x;}
int i=0;
void drawMyCar(GLfloat x,GLfloat y,GLfloat z,GLfloat angle){		//draw my car
	glRotatef(180.0f+angle, 0, 1.0f, 0);			//ROTATE ABOUT Y AND STRAIGHTEN THE CAR
	for(i=0;i<NUM;i++){					//test for collision
		if(mod(z-loc[i].z)<3&&mod(x-loc[i].x)<2){	//mycar within range of ai cars  COLLISION DETECTED
			speed=speed/2;		//reduce speed of mycar
			zpos+=0.5*(1-mod(yrot)/90);		//relocate my car
			xpos+=0.9*yrot/90;
			}
		}
	glTranslatef(0,0,0);
	RenderOBJModel (&mdl);
	glFlush();
	}

void drawAICars(GLfloat x,GLfloat z,int i){
	if(z==-100.0f&&airun[i]<=1.0f){z=-10.0f;airun[i]=90.0f*3.0f;}		//for aicars ahead in track
	if(z==-100.0f)z=-10.0f;
	if(z==-80.0f&&airun[i]<=1.0f){z=-20.0f;airun[i]=60.0f*3.0f;}		//for aicars ahead in track
	if(z==-80.0f)z=-20.0f;
	if(z==-140.0f&&airun[i]<=1.0f){z=-20.0f;airun[i]=120.0f*3.0f;}		//for aicars ahead in track
	if(z==-140.0f)z=-20.0f;
		//AI CARS
if(airun[i]<300*3.0){				//going straight
	//printf("airuni=%f\n",airun[i]);
/*glNormal3f( 0.0f, 0.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texture_car);
	glScalef(0.3,0.3,0.3);				//change size of car
	glTranslatef (1.0f, 0.0f, -10.0f-airun);airun+=0.8f;//change run to change speed   ytrans* changes perspective of car
	glRotatef(180.0f, 0, 1.0f, 0);			//ROTATE ABOUT Y AND STRAIGHTEN THE CAR
	RenderOBJModel (&mdl);
	glFlush();*/
	drawCar(x,0.0f,z-airun[i],0.0f);//if(i==0)printf("called for i=0 and airun[0]=%f\n",airun[0]);else printf("called for i=1\n");
	airun[i]+=0.8f;
}
else{
	if(angle[i]<90.0){					//ai car turn left
	/*glNormal3f( 0.0f, 0.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texture_car);
	glScalef(0.3,0.3,0.3);				//change size of car
	glTranslatef (1.0f, 0.0f, -10.0f-airun);airun+=0.5f;//change run to change speed   ytrans* changes perspective of car
	glRotatef(180.0f+angle, 0, 1.0f, 0);			//ROTATE ABOUT Y AND STRAIGHTEN THE CAR
	RenderOBJModel (&mdl);
	glFlush();	
	angle+=1.5;*/
	drawCar(x,0.0f,z-airun[i],angle[i]);
	angle[i]+=1.5;
	airun[i]+=1.2f;
	}
	else{						//drive left
		/*glNormal3f( 0.0f, 0.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texture_car);
		glScalef(0.3,0.3,0.3);				//change size of car
		glTranslatef (1.0f-airunx, 0.0f, -990.0f);airunx+=0.8f;//change run to change speed   ytrans* changes perspective of car
		glRotatef(180.0f+angle, 0, 1.0f, 0);			//ROTATE ABOUT Y AND STRAIGHTEN THE CAR
		RenderOBJModel (&mdl);
		glFlush();*/
		 if(airunx[i]<300.0/0.3f){drawCar(x-airunx[i], 0.0f, z-975.0f,angle[i]);
				airunx[i]+=0.8f;}
			
		     else{			
			   if(angle[i]<180.0){			//2nd left turn
				drawCar(x-airunx[i],0.0f,z-airun[i],angle[i]);
				angle[i]+=1.5;
				tmp[i]=airun[i];//airun+=1.2f;
				}
			   else{				//straight back
				if(tmp[i]>6.0){drawCar(x-airunx[i], 0.0f, z-tmp[i],angle[i]);
				tmp[i]-=1.2f;airunx2[i]=airunx[i];
					}
				else{				//3rd left turn
					if(angle[i]<270.0){			
					drawCar(x-airunx2[i],0.0f,z-tmp[i],angle[i]);
					angle[i]+=1.5;
					airunx2[i]-=0.2f;
						}
					else if(airunx2[i]>0.0f)		//straight right
					{drawCar(x-airunx2[i], 0.0f, z-tmp[i],angle[i]);
						airunx2[i]-=1.2f;}
				}
				}
			}
		}
}}
/* The main drawing function. */
GLvoid DrawGLScene(GLvoid)
{	keyOperations();   //perform key operations first
    GLfloat x_m, y_m, z_m, u_m, v_m;
    GLfloat xtrans, ztrans, ytrans;
    GLfloat sceneroty;
    int numtriangles;
	//display menu
  
    // calculate translations and rotations.
    xtrans = -xpos;
    ztrans = -zpos;
    ytrans = -walkbias-0.25f;
    sceneroty = 360.0f - yrot;
    	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
    glLoadIdentity();

    glRotatef(lookupdown, 1.0f, 0, 0);
    glRotatef(sceneroty, 0, 1.0f, 0);

    glTranslatef(xtrans, ytrans*6, ztrans);  		//translate TEMP  *8

//BACKGROUND
	glBindTexture(GL_TEXTURE_2D, texture_bg); //texture for background
	//ground coordinates
	glBegin(GL_QUADS);		
	glNormal3f( 0.0f, 0.0f, 1.0f);	
	glTexCoord2f(0,600); 
	glVertex3f(-400.0,-0.1,-400);
	glTexCoord2f(0,0); 
	glVertex3f(-400,-0.1,300);
	glTexCoord2f(600,0); 
	glVertex3f(300,-0.1,300);
	glTexCoord2f(600,600); 
	glVertex3f(300,-0.1,-400);
	glEnd();
    glBindTexture(GL_TEXTURE_2D, texture[filter]);    // pick the texture.		CHANGE HERE TO CHANGE IMAGE USED

    numtriangles = sector1.numtriangles;

    for (loop=0; loop<numtriangles; loop++) {        // loop through all the triangles 36 here
	glBegin(GL_QUADS);		
	glNormal3f( 0.0f, 0.0f, 1.0f);
	
	x_m = sector1.triangle[loop].vertex[0].x;
	y_m = sector1.triangle[loop].vertex[0].y;
	z_m = sector1.triangle[loop].vertex[0].z;
	u_m = sector1.triangle[loop].vertex[0].u;
	v_m = sector1.triangle[loop].vertex[0].v;
	glTexCoord2f(u_m,v_m); 
	glVertex3f(x_m,y_m,z_m);
	
	x_m = sector1.triangle[loop].vertex[1].x;
	y_m = sector1.triangle[loop].vertex[1].y;
	z_m = sector1.triangle[loop].vertex[1].z;
	u_m = sector1.triangle[loop].vertex[1].u;
	v_m = sector1.triangle[loop].vertex[1].v;
	glTexCoord2f(u_m,v_m); 
	glVertex3f(x_m,y_m,z_m);
	
	x_m = sector1.triangle[loop].vertex[2].x;
	y_m = sector1.triangle[loop].vertex[2].y;
	z_m = sector1.triangle[loop].vertex[2].z;
	u_m = sector1.triangle[loop].vertex[2].u;
	v_m = sector1.triangle[loop].vertex[2].v;
	glTexCoord2f(u_m,v_m); 
	glVertex3f(x_m,y_m,z_m);
	
	x_m = sector1.triangle[loop].vertex[3].x;
	y_m = sector1.triangle[loop].vertex[3].y;
	z_m = sector1.triangle[loop].vertex[3].z;
	u_m = sector1.triangle[loop].vertex[3].u;
	v_m = sector1.triangle[loop].vertex[3].v;
	glTexCoord2f(u_m,v_m); 
	glVertex3f(x_m,y_m,z_m);	

 }
	glEnd();



if(menu==0){
glPushMatrix();glPushMatrix();glPushMatrix();glPushMatrix();glPushMatrix();glPushMatrix();glPushMatrix();
drawAICars(-3.0,-10.0,1);glPopMatrix();
drawAICars(6.0,-20.0,0);glPopMatrix();
drawAICars(6.0,-20.0,3);glPopMatrix();
drawAICars(6.0,-80.0,4);glPopMatrix();
drawAICars(6.0,-140.0,5);glPopMatrix();
drawAICars(-2.0,-100.0,6);glPopMatrix();
drawAICars(-6.0,-20.0,2);
//drawAICars(3.0,-20.0);

//CAR
	glMatrixMode(GL_MODELVIEW) ;GLfloat yrotmod=yrot>=0?yrot:-yrot;
//glLoadIdentity();		
glPopMatrix();								//move car with road
//glNormal3f( 0.0f, 0.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texture_car);
	if(yrot>=360.0f){yrot=yrot-360;yrotmod=yrot-360;}		//complete revolutions to left
	if(yrot<=-360.0f){yrot=yrot+360;yrotmod=yrotmod-360;}		//complete revolutions to right
	glScalef(0.3,0.3,0.3);				//change size of car
	if(yrot>=90.0f&&yrot<180.0f)glTranslatef (carx=(xpos-4.0+4.0*(yrotmod-90)/90)/0.3, 0.0, carz=zpos/0.3-10+10*(yrotmod/90));//left turn>90
	else if(yrot>=180.0f&&yrot<270.0f)		//turn left >180
	glTranslatef (carx=(xpos+4*(yrotmod-180)/90)/0.3, 0.0,carz= zpos/0.3+10.0-10*((yrotmod-180.0f)/90));
	else if(yrot>=270.0f&&yrot<360.0f)		//turn left >270
	glTranslatef (carx=(xpos+4-4*(yrotmod-270)/90)/0.3, 0.0,carz= zpos/0.3+10.0-10*((yrotmod-180.0f)/90));
	
	else if(yrot<=-90.0f&&yrot>-180.0f)				//right turn>90
	glTranslatef (carx=(xpos+4.0-4.0*(yrotmod-90)/90)/0.3, 0.0,carz= zpos/0.3-10+10*(yrotmod/90));
	else if(yrot<=-180.0f&&yrot>-270.0f)				//right turn>180
	glTranslatef (carx=(xpos-4.0*(yrotmod-180)/90)/0.3, 0.0,carz= zpos/0.3+10-10*((yrotmod-180)/90));
	else if(yrot<=-270.0f&&yrot>-360.0f)				//right turn>270
	glTranslatef (carx=(xpos-4.0+4.0*(yrotmod-270)/90)/0.3, 0.0, zpos/0.3+10-10*((yrotmod-180)/90));
	//-90 to 90
	else glTranslatef (carx=(xpos-4.0*yrot/90)/0.3, 0.0,carz= zpos/0.3-10+10*(yrotmod/90));		//because of scale 0.3 divide zpos by 0.3
												//smooth change position while turning
	carx=(xpos-4.0*yrot/90)/0.3;carz=zpos/0.3-10+10*(yrotmod/90);
	//if(carx<-6.0f)carx=-6.0f;
	//glTranslatef(carx,0.0f,carz);									

	
	//else glTranslatef ((xpos-4.0)/0.3, 0.0, (zpos)/0.3);
//glTranslatef (0.0f, ytrans*6, -10.0f-run);//run+=0.1f;//change run to change speed   ytrans* changes perspective of
 /*glRotatef(180.0f+yrot, 0, 1.0f, 0);			//ROTATE ABOUT Y AND STRAIGHTEN THE CAR
	RenderOBJModel (&mdl);
	glFlush();*/
drawMyCar(carx,0.0,carz,yrot);



										//move car with road


	}//end if menu
	

   if(menu==1)
	{


	int i=0;
//glScalef(3.0,3.0,3.0);
glColor3f(1.0,0.0, 0.0);		//COLOR OF MENU OPTIONS GREEN
glRasterPos3d(-0.9,3.0,-15.0);

char *s1;s1="NEW GAME";
for (i = 0; i <= strlen(s1); i++)
	    {
	       glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s1[i]);
	    }
glRasterPos3d(-0.9,2.0,-15.0);

char *s2;s2="HIGH SCORE";
for (i = 0; i <= strlen(s2); i++)
	    {
	       glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s2[i]);
	    }
glRasterPos3d(-0.6,1.0,-15.0);

char *s3;s3="TRACK";
for (i = 0; i <= strlen(s3); i++)
	    {
	       glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s3[i]);
	    }
glRasterPos3d(-0.4,0.0,-15.0);

char *s4;s4="EXIT";
for (i = 0; i <= strlen(s4); i++)
	    {
	       glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s4[i]);
	    }

glColor3f(1.0,1.0, 1.0);
glFlush();
}

    // since this is double buffered, swap the buffers to display what just got drawn.
    glutSwapBuffers();
}



/* The function called whenever a normal key is pressed. */
void keyPressed(unsigned char key, int x, int y) 
{	
    
    usleep(100);
	keybuff[key]=1; //set key in buffer
    switch (key) {    
    case ESCAPE: // kill everything.
	/* exit the program...normal termination. */
	exit(1);                   	
	break; // redundant.
    case SPACE:menu=0;
		break;
    

    case 'f': glutFullScreen();
		break;
  
	case 'm':
	case 'M':
			menu=1;
			break;
   
    default:
      printf ("Key %d pressed. No action there yet.\n", key);
      break;
    }	
}

/* The function called whenever a normal key is pressed. */
void specialKeyPressed(int key, int x, int y) 
{	
  
    usleep(100);
	keybuff[key]=1; //set key in buffer
	//printf("menu=%d",menu);
   if(menu!=1){ 
	switch (key) {    
    case GLUT_KEY_PAGE_UP: // tilt up
	z -= 0.2f;
	lookupdown -= 0.2f;
	break;
    
    case GLUT_KEY_PAGE_DOWN: // tilt down
	z += 0.2f;
	lookupdown += 1.0f;
	break;

    case GLUT_KEY_UP: // walk forward 
	xpos -= (float)sin(yrot*piover180) * 0.5f;
	zpos -= (float)cos(yrot*piover180) * 0.2f;//0.05f;//10.0f;//0.05f	
	if (walkbiasangle >= 359.0f)
	    walkbiasangle = 0.0f;	
	//else 
	   // walkbiasangle+= 10;		wave effect
	//walkbias = (float)sin(walkbiasangle * piover180)/20.0f;
	break;

    case GLUT_KEY_DOWN: 
	xpos += (float)sin(yrot*piover180) * 0.05f;
	zpos += (float)cos(yrot*piover180) * 0.5f;//0.05f;//10.0f;	
	if (walkbiasangle <= 1.0f)
	    walkbiasangle = 359.0f;	
	else 
	    //walkbiasangle-= 10;
	walkbias = (float)sin(walkbiasangle * piover180)/20.0f;
	break;

    case GLUT_KEY_LEFT: // look left
	yrot += 1.5f;if(yrot>=90&&yrot<=180){straight=0;left=1;}	//to render car for left_right track
	break;
    
    case GLUT_KEY_RIGHT: // look right
	yrot -= 1.5f;if(yrot<=-90&&yrot>=-180){straight=0;left=1;}
	break;

    default:
	printf ("Special key %d pressed. No action there yet.\n", key);
	break;
    }	}
}

void keySpecialUp (int key, int x, int y) {
keybuff[key] = 0; // Set the state of the current key to not pressed
/*switch(key){
	//if up key
	case GLUT_KEY_PAGE_UP: break;
		//keep going for a while
	case GLUT_KEY_LEFT: // look left
	yrot =0.0f;break;
    
    case GLUT_KEY_RIGHT: // look right
	yrot =0.0f;break;

    default:
	printf ("Special key %d released. No action there yet.\n", key);
	break;	
		}*/
}

void keyUp (unsigned char key, int x, int y) {
keybuff[key] = 0; // Set the state of the current key to not pressed


}

void mouseClicks(int button, int state, int x, int y) {
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if(x>620&&x<770&&y>170&&y<190){			//New Game
		menu=0;printf("click %d, %d",x,y);
		}
	else if(x>620&&x<770&&y>240&&y<260){			//Track 625 248,773
		printf("click %d, %d",x,y);
		}
	else if(x>620&&x<770&&y>300&&y<320){			//High Score
		printf("click %d, %d",x,y);
		}
	else if(x>620&&x<770&&y>360&&y<380){			//exit
		printf("click %d, %d",x,y);
		exit(1);                   	
	}
    }
}
int start_game()//(int argc, char **argv) 
{  
    /* load our world from disk */
    SetupWorld();
	
    /* Initialize GLUT state  */  
   // glutInit(&argc, argv);  

     
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);  

    /* get a 640 x 480 window */
    glutInitWindowSize(640, 480);  

    /* the window starts at the upper left corner of the screen */
    glutInitWindowPosition(0, 0);  

    /* Open a window */  
    window = glutCreateWindow("Track");  

    /* Register the function to do all our OpenGL drawing. */
    glutDisplayFunc(&DrawGLScene);  

    /* Go fullscreen.  This is as soon as possible. */
   glutFullScreen();

    /* Even if there are no events, redraw our gl scene. */
    glutIdleFunc(&DrawGLScene); 

    /* Register the function called when our window is resized. */
    glutReshapeFunc(&ReSizeGLScene);

    /* Register the function called when the keyboard is pressed. */
    glutKeyboardFunc(&keyPressed);
	glutKeyboardUpFunc(&keyUp);

    /* Register the function called when special keys (arrows, page down, etc) are pressed. */
    glutSpecialFunc(&specialKeyPressed);
	glutSpecialUpFunc(&keySpecialUp);
	/*Mouse click function*/
	glutMouseFunc(mouseClicks);
    /* Initialize our window. */
    InitGL(640, 480);
  ReadOBJModel ("src/car.obj",&mdl);
	
    /* Start Event Processing Engine */  
    glutMainLoop();  
atexit (cleanup);
    return 1;
}
