#include <enet/enet.h>
#include "core.h"


bool main_run=1;

char EXE_DIR[1000];
HWND WINDOW_HWND; 
SDL_Surface *WINDOW_HWND_SDL_SURF=0;

float raycast_width  = 1024;//1024+512;//+512;// 1280;//1280;//1024+512;//+256;//+256;//+256;//1280;//1280;//1920;// 1280;//1920;//1280;///2;
float raycast_height = 512;//768;//640;//640;//+256;//768;//+256;//+256;//+256;//720;//720;//1024;// 720;// 1024;//768;///2;
int WINDOW_WIDTH_MAX  = 1920;//raycast_width;//1920;
int WINDOW_HEIGHT_MAX = 1080;//raycast_height;//1080;
int WINDOW_WIDTH  =raycast_width;//1920/2;
int WINDOW_HEIGHT =raycast_height;//1080/2;
int WINDOW_BPP = 32;

int USE_TERRAIN=1;

//vec4f sunlightvec(-1,1,2,0);
vec4f sunlightvec=normalize3(vec4f(-1,2,-0.25,0));
//vec4f sunlightvec=normalize3(vec4f(-1.0,1.0,-0.15,0));
vec4f sunlightvec_objspc;

vec4f color_effects;
float audio_level=0;


#include "win.h"
#include "Bmp.h"
#include "zip/miniz.h"
#include "file.h"

char palette_name[]="../data/spectrum.png";
Bmp  palette_bmp(palette_name,1);

float fps=1.0;

int FRAME=0;
int FRAMETIME=0;
char  KEYTAB[512];
char  KEYTAB_UP[512];
char  KEYTAB_DOWN[512];
float MOUSE_X=0;
float MOUSE_Y=0;
float MOUSE_DX=0;
float MOUSE_DY=0;
float MOUSE_WHEEL=0;
float MOUSE_DWHEEL=0; // delta wheel
char  MOUSE_BUTTON     [32];
char  MOUSE_BUTTON_DOWN[32];
char  MOUSE_BUTTON_UP  [32];

int ACTIVE_SCREEN=1; // 3d view / editor / ..
int MOUSE_CURSOR=0;

#include "ui.h"
#include "ocl.h"
#include "ogl.h"
#include "gui.h"
#include "Mesh.h"
#include "Terrain.h"
#include "octree/octree.h"
#include "octree/rle4.h"
#include "octree/tree.h"
#include "bvh.h"
#include "raycast.h"
#include "skybox.h"
#include "world.h"
#include "voxeledit.h"


namespace net
{
	using namespace std;
	using namespace glm;

	#include "net_rpc.h"
	#include "net_client.h"
	#include "net_server.h"

	////////////////////////////////////////////////////////////////////////////////
	// 
	// Global Function for Text rendering - used by game_client.h 
	//
	namespace global
	{
		bool		exit = false;
		const int	width = 75, height = 25;
		const char  level[height][width] =
		{
			{ "-Giant Dungeon Tutorial--------         ------------------------------" },//0
			{ "                                                                      " },//1
			{ "                                                                      " },//2
			{ "       x------+--------------------x                                  " },//3
			{ "              |                                                       " },//4
			{ "              |                                    x  x               " },//5
			{ "              |                                    |  |               " },//6
			{ "              x                                    |  |               " },//7
			{ "                          x  x                     x--x               " },//8
			{ "                          |  |                                        " },//9
			{ "                          |  |     x----x                             " },//10
			{ "                          |  |     |                                  " },//11
			{ "                  x-------x  |     x               x                  " },//12
			{ "                  |          |                     |                  " },//13
			{ "                  x---x x----x                     |      x----x      " },//14
			{ "                                                   |                  " },//15
			{ "                                                   |                  " },//16
			{ "                                              x----+-------x          " },//17
			{ "       /|_____                                                        " },//18
			{ "      /   /   |___                                                    " },//19
			{ " ____/   /     /  |_______                                            " },//20
			{ "              /                                                       " },//21
			{ "                                                                      " },//22
			{ "                                                                      " } //23
		};
		char	screen[height][width + 2];
		void	screen_set(int x, int y, char c)
		{
			if (x < 0 || x >= width || y < 0 || y >= height) return;
			screen[y][x] = c;
		}
		void	screen_set(int x, int y, string s)
		{
			loopi(0, s.length()) screen_set(x + i, y, s[i]);
		}
		void	screen_clear()
		{
			loopi(0, width) loopj(0, height) screen_set(i, j, ' ');
			loopj(0, height) screen[j][width] = '\n';
			loopj(0, height) screen[j][width + 1] = 0;
			screen[height - 1][width] = 0;
		};
		void	screen_draw_level()
		{
			//return;
			loopj(0, height) loopi(0, strlen(&(level[j][0])))
				screen_set(i, j, level[j][i]);
		};
		void	screen_draw()
		{
			string s;
			loopi(0, height)	s.append(&(screen[i][0]));
			core_console_draw(s);
		};
		bool	collision(int x, int y) { return level[y][x] == ' ' ? false : true; };
	};
	////////////////////////////////////////////////////////////////////////////////
#include "game_server.h"
#include "game_client.h"
};


float audio_buffer[1024];

void reshape(GLsizei w,GLsizei h)
{
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  ui.reshape(w, h); 
}

void audio_posteffect(void *udata, Uint8 *stream, int len)
{
	static float sintab[512]={100};
	if(sintab[0]>=50) 
		loopi(0,512) sintab[i]=sin(M_PI*float(i)/512.0f);
		
	float a=0;
	loopi(0,512)
	{
		char x=stream[1+i*4];
		float b=float((x))/128;

		if((i&7)==0)
			loopj(0,5) a+=fabs(b*sintab[int((j>>2)+i)&511]);

		audio_buffer[i]=b;
	}
	a/=80;
	clamp(a,0,1);
	audio_level=a*a;
}

void audio_init()
{
	//Audio
	/* We're going to be requesting certain things from our audio
		device, so we set them up beforehand */
	int audio_rate = 22050;
	Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
	int audio_channels = 2;
	int audio_buffers = 512;

	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) 
	{
		error_stop("Unable to open audio!\n");
	}
	Mix_SetPostMix(audio_posteffect, NULL);
}

void sdl_init()
{
    //Initialize SDL
    if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    {
        error_stop("SDL_Init failed\n");
    }

    //Create Window
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

	//SDL_Window *mainwindow = SDL_CreateWindow("huhu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      //  512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	
	WINDOW_HWND_SDL_SURF=SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, SDL_HWSURFACE | SDL_OPENGL | SDL_RESIZABLE );
    if( WINDOW_HWND_SDL_SURF == NULL )
    {
        error_stop("SDL_SetVideoMode failed\n");
    }

    //Enable unicode
    SDL_EnableUNICODE( SDL_TRUE );
		
    //Set caption
    SDL_WM_SetCaption( "OutStar", NULL );

	WINDOW_HWND=FindWindow(NULL, "OutStar");
}

void ui_init()
{
	if (!ui.init())
	{
        error_stop("UI initialization failed\n");
	}
	ui.reshape(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void music_init()
{
	Mix_Music *music = Mix_LoadMUS("../data/background.ogg"); //music.s3m
	if(music == NULL) 
	{
		error_stop("Unable to load Ogg file: %s\n", Mix_GetError()); 
	}
	//if(Mix_PlayMusic(music, -1) == -1) { printf("Unable to play Ogg file: %s\n", Mix_GetError()); return 1; }
	
	if(Mix_FadeInMusic(music, -1,1000) == -1)
	{
		error_stop("Unable to play Ogg file: %s\n", Mix_GetError()); 
	}
}

void toggle_fullscreen()
{
	static RECT r,rnow;
	static char fullscreen=0;
	HWND hwnd=WINDOW_HWND;
	fullscreen^=1;

	if(fullscreen)
	{
		GetWindowRect(hwnd,&r);
		SetWindowLong(hwnd,GWL_STYLE, WS_VISIBLE |  WS_EX_TOPMOST );
		ShowWindow(hwnd,SW_MAXIMIZE);

		//raycast_width = WINDOW_WIDTH_MAX;
		//raycast_height=	WINDOW_HEIGHT_MAX;
	}
	else
	{
		SetWindowLong(hwnd,GWL_STYLE, WS_VISIBLE | WS_EX_TOPMOST | WS_CAPTION |WS_SYSMENU| WS_POPUP |  WS_THICKFRAME); //WS_POPUP | 
		MoveWindow(hwnd,r.top,r.left,r.right-r.left,r.bottom-r.top,0);

		//raycast_width = (WINDOW_WIDTH/64)*64;
		//raycast_height=	(WINDOW_HEIGHT/64)*64;
	}

	GetWindowRect(hwnd,&rnow);
	WINDOW_WIDTH=rnow.right-rnow.left;
	WINDOW_HEIGHT=rnow.bottom-rnow.top;
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
}

void handleKeys( int sym, int key, int x, int y )
{
	//printf("key %d sym %d\n",key,sym);
    if( sym == SDLK_ESCAPE ) main_run=false;//exit(0);
	if( sym == SDLK_F1 ) ogl_screenshot();
    if( sym == SDLK_F2 ) toggle_fullscreen();
	ui.keyboard(key, x, y);
}

void main_fps()
{
	// calc fps
	static int time0=timeGetTime(); 
	int time1=FRAMETIME=timeGetTime();
	float delta=float(int(time1-time0));
	time0=time1;
	if(delta>0)	fps=fps*0.9+100.0/delta;
}

void mouse_cursor(bool onoff)
{
	SDL_ShowCursor(onoff);
	//SDL_WM_GrabInput(SDL_GRAB_OFF:SDL_GRAB_ON );
	MOUSE_CURSOR=onoff;
	SDL_WarpMouse(WINDOW_WIDTH/2,WINDOW_HEIGHT/2);
}

void update_mouse_keyb()
{
	static float mx=MOUSE_X;
	static float my=MOUSE_Y;
	MOUSE_DX=MOUSE_X-mx;
	MOUSE_DY=MOUSE_Y-my;
	mx=MOUSE_X;
	my=MOUSE_Y;

	MOUSE_DWHEEL=MOUSE_WHEEL;
	MOUSE_WHEEL=0;

	static int mb[32]={0,0,0,0,0};

	loopi(0,32)
	{
		MOUSE_BUTTON_DOWN[i]=( MOUSE_BUTTON[i]) & (!mb[i]);
		MOUSE_BUTTON_UP  [i]=(!MOUSE_BUTTON[i]) & ( mb[i]);
		mb[i]=MOUSE_BUTTON[i];
	}

	static int kt[512]={-10}; if(kt[0]==-10) loopi(0,512)kt[i]=0; 

	loopi(0,512)
	{
		KEYTAB_DOWN[i]=( KEYTAB[i]) & (!kt[i]);
		KEYTAB_UP  [i]=(!KEYTAB[i]) & ( kt[i]);
		kt[i]=KEYTAB[i];
	}

	if(MOUSE_BUTTON_DOWN[1]){ mouse_cursor(ACTIVE_SCREEN);ACTIVE_SCREEN^=1;}
	if(KEYTAB[SDLK_F3]){ ACTIVE_SCREEN=0;mouse_cursor(1);}
	if(KEYTAB[SDLK_F4]){ ACTIVE_SCREEN=1;mouse_cursor(0);}
}

bool loading=1;

void splash_screen()
{
	static int tex = ogl_tex_bmp("../data/splash.png");

	ogl_bind(0,tex);
	
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
	ogl_drawquad(-1,-1,1,1,0,1,1,0);

	wglSwapIntervalEXT(0);
    SDL_GL_SwapBuffers();
}

void render()
{
	main_fps();
	update_mouse_keyb();

	color_effects.x=audio_level;
	color_effects.y=frac(color_effects.y+0.1*audio_level);//float((((timeGetTime()>>7)^345)%234)&128)/128.0f;
	color_effects.z=float(((timeGetTime()>>4)^6)&12)/12.0f;
	color_effects.w=sin( float(timeGetTime()&2047)/2047.0*2*M_PI )*0.3+0.7;
	
    //Clear color buffer
	if(ACTIVE_SCREEN==0) voxeledit::draw();
	if(ACTIVE_SCREEN==1) world::update(raycast_width,raycast_height);

	/*
	audio_level=1;
	glBegin( GL_QUADS );
		glVertex2f( -1.0f, -1.0f );
		glVertex2f(  audio_level*2-1, -1.0f );
		glVertex2f(  audio_level*2-1,  -0.8f );
		glVertex2f( -1.0f,  -0.8f );
	glEnd();
	*/
	
	/*
    glBegin( GL_LINE_STRIP);
	for(int i=0;i<512;i++)
        glVertex2f( float(i)/256.0-1, audio_buffer[i] );
    glEnd();
	*/

	//doUI();

    //Update screen
	wglSwapIntervalEXT(0);
    SDL_GL_SwapBuffers();
	FRAME++;
}


int main( int argc, char *argv[] )
{

	//ShowWindow( GetConsoleWindow(), SW_HIDE );
	{
		strcpy(EXE_DIR,argv[0]);
		int ofs=0;
		for (int i=0;i<strlen(EXE_DIR);i++) 
		{
			//if(EXE_DIR[i]=='\\') EXE_DIR[i]='/';
			if(EXE_DIR[i]=='\\') ofs = i;
		}
		EXE_DIR[ofs]=0;
		//SetCurrentDirectory("c:\\");//exe_dir);
		printf("EXE_DIR %s\n",EXE_DIR);
	}

	// Init > Begin
	memset(MOUSE_BUTTON,0,32);
	memset(KEYTAB,0,512);

	sdl_init();
    ogl_init();

	//toggle_fullscreen();
	splash_screen();

	ocl_init();
	ui_init();
	terrain::init();
	raycast::init();
	audio_init();
	//music_init();

	mouse_cursor(ACTIVE_SCREEN^1);

	// Init > End
	
	//mciSendString("open \"..\\sound.wav\" type MPEGVideo alias bgmusic", NULL, 0, 0); 
	//mciSendString("open \"url://listen.technobase.fm/tunein-mp3-pls\" type MPEGVideo alias bgmusic", NULL, 0, 0); 
	//mciSendString("play bgmusic notify repeat", NULL, 0, 0);
	//mciSendString("setaudio bgmusic volume to 1000", 0, 0, 0);		
	//mciSendString("open \"..\\sound.wav\" type MPEGVideo alias bgmusic", NULL, 0, 0); 	
	
	loading=0;

	//mainloop
	while(main_run)
	{
        //While there are events to handle
		static SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			uint key_id=event.key.keysym.sym;

			if( event.type == SDL_QUIT ) main_run=0;
            if( event.type == SDL_KEYUP )
            {
				if(key_id>=512) error_stop("SDL_KEYUP key %d out of bound 512",key_id);
				KEYTAB[key_id]=0;
			}
            if( event.type == SDL_KEYDOWN )
            {
				if(key_id>=512) error_stop("SDL_KEYDOWN key %d out of bound 512",key_id);
				KEYTAB[key_id]=1;
                //Handle keypress with current mouse position
                int x = 0, y = 0;
                SDL_GetMouseState( &x, &y );
				handleKeys( key_id, event.key.keysym.unicode, x, y );
            }
            if( event.type == SDL_MOUSEMOTION )
            {
				if(MOUSE_CURSOR)
				{
					ui.mouseMotion(event.motion.x, event.motion.y);
					MOUSE_X=event.motion.x;
					MOUSE_Y=event.motion.y;
				}
            }
            if( event.type == SDL_MOUSEBUTTONDOWN )
            {
				//mciSendString("seek bgmusic TO START", NULL, 0, 0);
				//mciSendString("play bgmusic", NULL, 0, 0);
				//printf("mb %d \n",event.button.button);
				MOUSE_BUTTON[event.button.button-1]=1;
				ui.mouse(event.button.button-1, GLUT_DOWN, event.motion.x, event.motion.y);

				if( event.button.button == SDL_BUTTON_WHEELUP )
				{
					MOUSE_WHEEL-=1;
				}
			}
            if( event.type == SDL_MOUSEBUTTONUP )
            {
				MOUSE_BUTTON[event.button.button-1]=0;
				ui.mouse(event.button.button-1, GLUT_UP, event.motion.x, event.motion.y);

				if( event.button.button == SDL_BUTTON_WHEELDOWN )
				{
					MOUSE_WHEEL+=1;
				}	
			}
			
            if( event.type == SDL_VIDEORESIZE )
			{        
				SDL_FreeSurface(WINDOW_HWND_SDL_SURF);
				WINDOW_HWND_SDL_SURF = SDL_SetVideoMode(event.resize.w,event.resize.h,32,SDL_HWSURFACE |SDL_OPENGL|SDL_RESIZABLE);
				reshape(static_cast<GLsizei>(event.resize.w),static_cast<GLsizei>(event.resize.h));
				WINDOW_WIDTH =event.resize.w;
				WINDOW_HEIGHT=event.resize.h;
			}
		}

		if(MOUSE_CURSOR==0)
		if(WINDOW_HWND==GetForegroundWindow())
		{
			//Somewhere in the gameloop
			int MouseX,MouseY;
			SDL_GetMouseState(&MouseX,&MouseY);
			MOUSE_X+= MouseX - WINDOW_WIDTH/2;
			MOUSE_Y+= MouseY - WINDOW_HEIGHT/2;
			SDL_WarpMouse(WINDOW_WIDTH/2,WINDOW_HEIGHT/2);
		}

		//Render frame
        render();

	}

	//Clean up
	raycast::quit();
	Sleep(1000);
    //SDL_Quit();

	return 0;
}
/*
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)			// Anyway, so here is our "main()" for windows.  Must Have this for a windows app.
{
	return main(1,&szCmdLine);
}
*/