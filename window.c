/*!\file window.c
 *
 * \brief utilisation de GL4Dummies et Lib Assimp pour chargement de
 * modèles 3D.
 *
 * Ici on charge un modèle d'avion et on le fait tourner sur un cercle
 * en le suivant du regard.
 *
 * \author Farès Belhadj amsi@ai.univ-paris8.fr
 * \date May 13 2018
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4df.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>

/*!\brief largeur de la fenêtre */
static int _windowWidth = 800;
/*!\brief haiteur de la fenêtre */
static int _windowHeight = 600;
/*!\brief largeur de la heightMap générée */
static int _landscape_w = 1025;
/*!\brief heuteur de la heightMap générée */
static int _landscape_h = 1025;
/*!\brief scale en x et z du modèle de terrrain */
static GLfloat _landscape_scale_xz = 100.0f;
/*!\brief scale en y du modèle de terrain */
static GLfloat _landscape_scale_y = 10.0f;
/*!\brief heightMap du terrain généré */
static GLfloat * _heightMap = NULL;
/*!\brief identifiant d'une sphère (soleil) */
/*!\brief identifiant du terrain généré */
static GLuint _landscape = 0;
/*!\brief identifiant GLSL program du terrain */
static GLuint _landscape_pId  = 0;
/*!\brief identifiant GLSL program du soleil */
/*!\brief identifiant de la texture de dégradé de couleurs du terrain */
static GLuint _terrain_tId = 0;
/*!\brief identifiant de la texture de dégradé de couleurs du soleil */
static GLuint _yellowred_tId = 0;
/*!\brief identifiant de la texture de plasma */
static GLuint _plasma_tId = 0;
static int _wW = 800;
/*!\brief opened window height */
static int _wH = 600;
/*!\brief paramètres du cercle suivi par l'avion */
static GLfloat _radius = 10, _x0 = 10, _z0 = -10, _y0 = 2.5;
/*!\brief paramètres de l'avion */
static GLfloat _x = 0, _y = 0, _z = 0, _alpha = 0;
/*!\brief GLSL program Id */
static GLuint _pId = 0, _pId2 = 0, _pId3 = 0;
/* une sphere pour la bounding sphere */
static GLuint _sphere = 0, _quad = 0 ,_cube = 0, _snow = 0;
/* une texture pour les nuages */
static GLuint _tId = 0;
static int _xm = 400, _ym = 300;
static GLboolean _anisotropic = GL_FALSE;
static float _p = 20;
static int initial = 0;
static int mx[1000];
static int mz[1000];
static int my[1000];
static int colx[100];
static int colz[100];
static int maxcol=15;
static int nbcol=0;
static int snowing = 0;
static GLuint _planeTexId = 0;
static GLboolean _fog = GL_FALSE;
static GLboolean _mipmap = GL_FALSE;
static GLuint _sun_pId = 0;
static GLfloat _cycle = 0.0f;
static GLfloat _cycle2 = 0.0f;

extern void assimpInit(const char * filename);
extern void assimpDrawScene(void);
extern void assimpQuit(void);
static void keydown(int keycode);
static void keyup(int keycode);
static void pmotion(int x, int y);
static void init(void);
static void quit(void);
static void resize(int w, int h);
static void idle(void);
static void draw(void);
static GLfloat heightMapAltitude(GLfloat x, GLfloat z);


enum kyes_t {
  KLEFT = 0,
  KRIGHT,
  KUP,
  KDOWN
};
static GLuint _keys[] = {0, 0, 0, 0};


typedef struct cam_t cam_t;
/*!\brief a data structure for storing camera position and
 * orientation */
struct cam_t {
  GLfloat x, z;
  GLfloat theta;
};

static cam_t _cam = {0, 0, 0};


int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", 0, 0,
                         _wW, _wH, GL4DW_RESIZABLE | GL4DW_SHOWN))
    return 1;
  /* charge l'avion */
  assimpInit("models/fish/fish.obj");
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwDisplayFunc(draw);
  gl4duwKeyUpFunc(keyup);
  gl4duwKeyDownFunc(keydown);
  gl4duwPassiveMotionFunc(pmotion);
  gl4duwIdleFunc(idle);
  gl4duwMainLoop();
  return 0;
}

static void loadTexture(GLuint id, const char * filename) {
  SDL_Surface * t;
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if( (t = IMG_Load(filename)) != NULL ) {
#ifdef __APPLE__
    int mode = t->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
#else
    int mode = t->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
#endif       
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0, mode, GL_UNSIGNED_BYTE, t->pixels);
    SDL_FreeSurface(t);
  } else {
    fprintf(stderr, "can't open file %s : %s\n", filename, SDL_GetError());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }
}

static void init(void) {
    SDL_Surface * t;
  /* pour générer une chaine aléatoire différente par exécution */
  srand(time(NULL));
  /* paramètres GL */
  glClearColor(0.0f, 0.4f, 0.9f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  /* chargement et compilation des shaders */
  _landscape_pId  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _pId = gl4duCreateProgram("<vs>shaders/model.vs", "<fs>shaders/model.fs", NULL);
  _pId2 = gl4duCreateProgram("<vs>shaders/model.vs", "<fs>shaders/clouds.fs", NULL);
  _pId3  = gl4duCreateProgram("<vs>shaders/dep3d.vs", "<fs>shaders/dep3d.fs", NULL);
  _sun_pId = gl4duCreateProgram("<vs>shaders/balo.vs", "<fs>shaders/sun.fs", NULL);
  /* création des matrices de model-view et projection */
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  _sphere = gl4dgGenSpheref(30, 30);
  _quad = gl4dgGenQuadf();
  _cube = gl4dgGenCubef();
  _snow = gl4dgGenSpheref(10,10);
  glGenTextures(1, &_tId);
  loadTexture(_tId, "images/nuages.jpg");
   resize(_windowWidth, _windowHeight);

  _heightMap = gl4dmTriangleEdge(_landscape_w, _landscape_h, 0.6);
  /* création de la géométrie du terrain en fonction de la heightMap */
  _landscape = gl4dgGenGrid2dFromHeightMapf(_landscape_w, _landscape_h, _heightMap);

  glBindTexture(GL_TEXTURE_1D, _terrain_tId);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t = IMG_Load("neige.png");
  assert(t);
#ifdef __APPLE__
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
#else
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
#endif
  SDL_FreeSurface(t);
}


/*!\brief function called by GL4Dummies' loop at resize. Sets the
 *  projection matrix and the viewport according to the given width
 *  and height.
 * \param w window width
 * \param h window height
 */
static void resize(int w, int h) {
  glViewport(0, 0, _windowWidth = w, _windowHeight = h);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, 1000.0);
}

/*!\brief function called by GL4Dummies' loop at idle.
 * 
 * uses the virtual keyboard states to move the camera according to
 * direction, orientation and time (dt = delta-time)
 */

 static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}
GLfloat dayN=0.5f;
GLfloat dayL=0.4f;
int count=0,tour, ok=0;


int colision (){
  for(int i = 0 ; i < nbcol ; i++){
    if ((int)_cam.x-colx[i] > -2.5 && (int)_cam.x-colx[i] < 2.5 && (int)_cam.z-colz[i] > -2.5 && (int)_cam.z-colz[i] < 2.5)
      return 1;
  }
  return 0;
}

/*!\brief simulation : modification des paramètres de la caméra
 * (LookAt) en fonction de l'interaction clavier */
static void idle(void) {
  double dt, dtheta = M_PI, pas = 15.0;
  dt = get_dt();
  //_cycle += dt;
  if((count>=10)&&(count<=1000)){
    _cycle += dt;
    tour++;
    count=0;
  if((tour>=45)&&(ok!=1)){
      dayN=dayN-0.02;      
      dayL=dayL-0.02;
      //printf("B::%lf\nG::%lf\n",dayN,dayL);
      if((dayN>=0.12)&&(dayL>=0.02)){
        glClearColor(0.0f, dayL, dayN, 0.0f);
      }
  }
  if(tour==140){
    dayN=0.12;dayL=0.02;
  }
  if(tour>160){
    ok=1;
    dayN=dayN+0.02;      
    dayL=dayL+0.02;
      //printf("B::%lf\nG::%lf\n",dayN,dayL);
      if((dayN<=0.5)&&(dayL<=0.4)){
            glClearColor(0.0f, dayL, dayN, 0.0f);
      }
  }

  if(tour==200){
    tour=0;
    ok=0;
  }
    //printf("------>>>>>%d\n",tour);
  }
  //if(count>75 && count<150)
  if(_keys[KLEFT])
    _cam.theta += dt * dtheta;
  if(_keys[KRIGHT])
    _cam.theta -= dt * dtheta;
  if(_keys[KUP]) {
    count+=3;
    if(heightMapAltitude(_cam.x += -dt * pas * sin(_cam.theta), _cam.z += -dt * pas * cos(_cam.theta))
      < heightMapAltitude(0,0)-3 || colision()==1){
      _cam.x += dt * pas * sin(_cam.theta);
    _cam.z += dt * pas * cos(_cam.theta);
    }
  }
  if(_keys[KDOWN]) {
    count+=3;
    if (heightMapAltitude(_cam.x += dt * pas * sin(_cam.theta), _cam.z += dt * pas * cos(_cam.theta))
      < heightMapAltitude(0,0)-3 || colision()==1){
      _cam.x += -dt * pas * sin(_cam.theta);
    _cam.z += -dt * pas * cos(_cam.theta);
  }}
  double lt, t;
  static Uint32 p0 = 0, p;
  lt = ((t = SDL_GetTicks()) - p0) / 100000.0;
  p0 = p;
  _alpha -= 0.1f * lt;
  _x = _x0 + _radius * cos(_alpha);
  _z = _z0 - _radius * sin(_alpha);
  _y = _y0;
}




static void keydown(int keycode) {
  GLint v[2];
  switch(keycode) {
  case GL4DK_LEFT:
    _keys[KLEFT] = 1;
    break;
  case GL4DK_RIGHT:
    _keys[KRIGHT] = 1;
    break;
  case GL4DK_UP:
    _keys[KUP] = 1;
    break;
  case GL4DK_DOWN:
    _keys[KDOWN] = 1;
    break;
  case GL4DK_ESCAPE:
  case 'q':
    exit(0);
    /* when 'w' pressed, toggle between line and filled mode */
  case 'w':
    glGetIntegerv(GL_POLYGON_MODE, v);
    if(v[0] == GL_FILL) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glLineWidth(3.0);
    } else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glLineWidth(1.0);
    }
    break;
  case 'f':
    _fog = !_fog;
    break;
    /* when 'm' pressed, toggle between mipmapping or nearest for the plane texture */
  case 'm': {
    _mipmap = !_mipmap;
    glBindTexture(GL_TEXTURE_2D, _planeTexId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    break;
  }
    /* when 'a' pressed, toggle on/off the anisotropic mode */
  case 'a': {
    _anisotropic = !_anisotropic;
    /* l'Anisotropic sous GL ne fonctionne que si la version de la
       bibliothèque le supporte ; supprimer le bloc ci-après si
       problème à la compilation. */
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
    GLfloat max;
    glBindTexture(GL_TEXTURE_2D, _planeTexId);
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _anisotropic ? max : 1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
    break;
  }
  default:
    break;
  }
}

/*!\brief function called by GL4Dummies' loop at key-up (key
 * released) event.
 * 
 * stores the virtual keyboard states (0 = released).
 */
static void keyup(int keycode) {
  switch(keycode) {
  case GL4DK_LEFT:
    _keys[KLEFT] = 0;
    break;
  case GL4DK_RIGHT:
    _keys[KRIGHT] = 0;
    break;
  case GL4DK_UP:
    _keys[KUP] = 0;
    break;
  case GL4DK_DOWN:
    _keys[KDOWN] = 0;
    break;
  default:
    break;
  }
}

/*!\brief function called by GL4Dummies' loop at the passive mouse motion event.*/
static void pmotion(int x, int y) {
  _xm = x; 
  _ym = y;
}
int rand_a_b(int a, int b){
    return rand()%(b-a) +a;
}

void init_meteo (){
  for (int i = 0 ; i < 1000 ; i++){
    mx[i] = rand_a_b(-100,100);
    mz[i] = rand_a_b(-100,100);
    my[i] = rand_a_b(0,100);
  }

  initial=1;
}

void creat_tree (int x, int z){
GLfloat vert[] = {0, 1, 0, 1}, marron[] = {0.47, 0.2, 0.07, 1}, blanc[] = {1, 1, 1, 1};
    gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(x, heightMapAltitude(x,z), z);
    gl4duRotatef(0, 0, 0, 0);
    gl4duScalef(1, 7, 1);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, marron);
  gl4dgDraw(_cube);

      gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(x, heightMapAltitude(x,z)+7, z);
    gl4duRotatef(0, 0, 0, 0);
    gl4duScalef(4, 4, 4);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, vert);
  gl4dgDraw(_sphere);

        gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(x, heightMapAltitude(x,z)+8, z);
    gl4duRotatef(0, 0, 0, 0);
    gl4duScalef(4, 4, 4);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, blanc);
  gl4dgDraw(_sphere);
    if (nbcol < maxcol){
    colx[nbcol] = x;
    colz[nbcol] = z;
    nbcol++;
  }
}

void creat_snowman (int x, int z){
GLfloat vert[] = {0, 1, 0, 1}, red[] = {1, 0, 0, 1}, blanc[] = {1, 1, 1, 1};

      gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(x, heightMapAltitude(x,z)+0.7, z);
    gl4duRotatef(0, 0, 0, 0);
    gl4duScalef(1, 1, 1);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, blanc);
  gl4dgDraw(_sphere);

        gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(x, heightMapAltitude(x,z)+2.2, z);
    gl4duRotatef(0, 0, 0, 0);
    gl4duScalef(0.6, 0.6, 0.6);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, blanc);
  gl4dgDraw(_sphere);

          gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(x+0.5, heightMapAltitude(x+0.5,z)+2.2, z);
    gl4duRotatef(-180, -180, -180, 0);
    gl4duScalef(0.1, 0.6, 0.1);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, red);
  gl4dgDraw(_sphere);

  if (nbcol < maxcol){
    colx[nbcol] = x;
    colz[nbcol] = z;
    nbcol++;
  }
}


void creat_grass (int x, int z){
  GLfloat vert[] = {0, 1, 0, 1};

    gl4duPushMatrix(); {
    glUseProgram(_pId3);
    gl4duTranslatef(x, heightMapAltitude(x,z), z);
    gl4duRotatef(20, 0, 0, 0);
    gl4duScalef(0.01, 0.5, 0.01);
    gl4duSendMatrices();
  } gl4duPopMatrix();
glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, vert);
  gl4dgDraw(_cube);
}

static void draw(void) {
  if (initial == 0)
    init_meteo();
  

  int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
  GLfloat landscape_y;
  landscape_y = heightMapAltitude(_cam.x, _cam.z);
  GLfloat lum[4] = {0.0, 0.0, 5.0, 1.0};
  GLfloat rouge[] = {1, 0, 0, 1}, vert[] = {0, 1, 0, 1}, bleu[] = {0, 0, 1, 100}, jaune[] = {1, 1, 0, 1}, blanc[] = {1, 1, 1, 1};
  //glClearColor(0.0f, 0.7f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(_pId2);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _tId);
  glUniform1i(glGetUniformLocation(_pId, "tex"), 0);

  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  /* je regarde l'avion depuis 1, 0, -1 */
  gl4duLookAtf(_cam.x, landscape_y + 2.0, _cam.z, 
         _cam.x - sin(_cam.theta), landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
         0.0, 1.0,0.0);

/*  gl4duPushMatrix(); {
    gl4duScalef(400, 400, 400);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glDisable(GL_CULL_FACE);
  /* je dessine la bounding sphere avec _pId2 */
  //gl4dgDraw(_sphere);

  glUseProgram(_landscape_pId);
  gl4duPushMatrix(); {
      gl4duScalef(_landscape_scale_xz, _landscape_scale_y, _landscape_scale_xz);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glDisable(GL_CULL_FACE);
  /* je dessine la bounding sphere avec _pId2 */
  glBindTexture(GL_TEXTURE_1D, _terrain_tId);
  gl4dgDraw(_landscape);

  creat_tree(3,8);
  creat_tree(1,30);
  creat_tree(31,10);
  creat_tree(-3,-8);
  creat_tree(1,-30);
  creat_tree(-31,10);
  creat_snowman(35,5);
  creat_snowman(5,40);
  creat_snowman(0,5);
// printf("%d\n", colision());
// printf("%d\n",(int)_cam.x-colx[0]);
  gl4duPushMatrix(); {
 glUseProgram(_pId3);
    gl4duTranslatef(40, heightMapAltitude(0,0)-3, 0.0);
    gl4duRotatef(-90, 1, 0, 0);
    gl4duScalef(2000, 2000, 2000);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glUniform4fv(glGetUniformLocation(_pId3, "couleur"), 1, bleu);
  gl4dgDraw(_quad);

if (snowing==1){
  for (int i = 0 ; i < 1000 ; i++){

    if (my[i] > -20){
    gl4duPushMatrix(); {
    glEnable(GL_CULL_FACE);
    my[i] -= 1;
    gl4duTranslatef(mx[i]+_cam.x, my[i], mz[i]+_cam.z);
    gl4duRotatef(180, 180, 180, 0);
    gl4duScalef(1, 1, 1);
  	gl4duSendMatrices();
  	assimpDrawScene();
  } gl4duPopMatrix();
}
   else{
      my[i] = 80;
    }
  } 
}
  if((int)tour> 50){
  	snowing = 1;
  	}
  for (int i = 0 ; i < 1000 ; i++){
      creat_grass(mx[i],mz[i]);
}


glUseProgram(_sun_pId);
    gl4duPushMatrix(); {
    //gl4duTranslatef(temp[0], temp[1], temp[2]);
    gl4duTranslatef(-700, 150, -700);
    gl4duScalef(15, 15, 15);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glUniform1f(glGetUniformLocation(_sun_pId, "cycle"), _cycle);
  glUniform1i(glGetUniformLocation(_sun_pId, "plasma"), 0);
  glUniform1i(glGetUniformLocation(_sun_pId, "lave"), 1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _plasma_tId);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _yellowred_tId);
  gl4dgDraw(_sphere);
  glBindTexture(GL_TEXTURE_1D, 0);
  glActiveTexture(GL_TEXTURE0);

}

static void quit(void) {
  if(_heightMap) {
    free(_heightMap);
    _heightMap = NULL;
  }
  if(_plasma_tId) {
    glDeleteTextures(1, &_plasma_tId);
    _plasma_tId = 0;
    glDeleteTextures(1, &_yellowred_tId);
    _yellowred_tId = 0;
    glDeleteTextures(1, &_terrain_tId);
    _terrain_tId = 0;
  }
  gl4duClean(GL4DU_ALL);
}


static GLfloat heightMapAltitude(GLfloat x, GLfloat z) {
  x = (_landscape_w >> 1) + (x / _landscape_scale_xz) * (_landscape_w / 2.0); 
  z = (_landscape_h >> 1) - (z / _landscape_scale_xz) * (_landscape_h / 2.0);
  if(x >= 0.0 && x < _landscape_w && z >= 0.0 && z < _landscape_h)
    return (2.0 * _heightMap[((int)x) + ((int)z) * _landscape_w] - 1) * _landscape_scale_y;
  return 0;
}

