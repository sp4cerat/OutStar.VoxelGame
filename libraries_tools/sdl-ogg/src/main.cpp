/*This source code copyrighted by Lazy Foo' Productions (2004-2012)
and may not be redistributed without written permission.*/

//The headers
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
#include "SDL/SDL_opengl.h"
#include "SDL/SDL_mixer.h"

#pragma comment(lib,"native_midi.lib")
#pragma comment(lib,"timidity.lib")
#pragma comment(lib,"SDL_mixer.lib")

//Screen attributes
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

float audio_level=0;
float audio_buffer[1024];

//The frame rate
const int FRAMES_PER_SECOND = 60;

//Event handler
SDL_Event event;

//Rendering flag
bool renderQuad = true;

//The timer
class Timer
{
    private:
    //The clock time when the timer started
    int startTicks;

    //The ticks stored when the timer was paused
    int pausedTicks;

    //The timer status
    bool paused;
    bool started;

    public:
    //Initializes variables
    Timer();

    //The various clock actions
    void start();
    void stop();
    void pause();
    void unpause();

    //Gets the timer's time
    int get_ticks();

    //Checks the status of the timer
    bool is_started();
    bool is_paused();
};

bool initGL()
{
    //Initialize Projection Matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    //Initialize Modelview Matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    //Initialize clear color
    glClearColor( 0.f, 0.f, 0.f, 1.f );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR )
    {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        return false;
    }

    return true;
}

bool init()
{
    //Initialize SDL
    if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    {
        return false;
    }

    //Create Window
    if( SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_OPENGL ) == NULL )
    {
        return false;
    }

    //Enable unicode
    SDL_EnableUNICODE( SDL_TRUE );

    //Initialize OpenGL
    if( initGL() == false )
    {
        return false;
    }

    //Set caption
    SDL_WM_SetCaption( "OpenGL Test", NULL );

    return true;
}

void handleKeys( unsigned char key, int x, int y )
{
    //Toggle quad
    if( key == 'q' )
    {
        renderQuad = !renderQuad;
    }
}

void update()
{

}

void render()
{
    //Clear color buffer
    glClear( GL_COLOR_BUFFER_BIT );

    //Render quad
    if( renderQuad == true )
    {

        glBegin( GL_QUADS );
            glVertex2f( -1.0f, -1.0f );
            glVertex2f(  audio_level*2-1, -1.0f );
            glVertex2f(  audio_level*2-1,  -0.8f );
            glVertex2f( -1.0f,  -0.8f );
        glEnd();
        glBegin( GL_LINE_STRIP);
		for(int i=0;i<512;i++)
            glVertex2f( float(i)/256.0-1, audio_buffer[i] );
        glEnd();
	}

    //Update screen
    SDL_GL_SwapBuffers();
}

void clean_up()
{
    //Quit SDL
    SDL_Quit();
}

Timer::Timer()
{
    //Initialize the variables
    startTicks = 0;
    pausedTicks = 0;
    paused = false;
    started = false;
}

void Timer::start()
{
    //Start the timer
    started = true;

    //Unpause the timer
    paused = false;

    //Get the current clock time
    startTicks = SDL_GetTicks();
}

void Timer::stop()
{
    //Stop the timer
    started = false;

    //Unpause the timer
    paused = false;
}

void Timer::pause()
{
    //If the timer is running and isn't already paused
    if( ( started == true ) && ( paused == false ) )
    {
        //Pause the timer
        paused = true;

        //Calculate the paused ticks
        pausedTicks = SDL_GetTicks() - startTicks;
    }
}

void Timer::unpause()
{
    //If the timer is paused
    if( paused == true )
    {
        //Unpause the timer
        paused = false;

        //Reset the starting ticks
        startTicks = SDL_GetTicks() - pausedTicks;

        //Reset the paused ticks
        pausedTicks = 0;
    }
}

int Timer::get_ticks()
{
    //If the timer is running
    if( started == true )
    {
        //If the timer is paused
        if( paused == true )
        {
            //Return the number of ticks when the timer was paused
            return pausedTicks;
        }
        else
        {
            //Return the current time minus the start time
            return SDL_GetTicks() - startTicks;
        }
    }

    //If the timer isn't running
    return 0;
}

bool Timer::is_started()
{
    return started;
}

bool Timer::is_paused()
{
    return paused;
}
int musicPlaying=0;

void musicFinished() { musicPlaying = 0; };

#include "MMSystem.h"
#include "math.h"
	
// make a passthru processor function that does nothing...
void noEffect(void *udata, Uint8 *stream, int len)
{
	float a=0;
	for(int i=0;i<512;i++)
	{
		char x=stream[1+i*4];
		float b=float((x))/128;

		for(int j=0;j<5;j++)
		{
			a+=fabs(b*(sin(3.141626*float(j)/5+3.141626*float(i)/100.0f)));
			//a+=fabs(b*(sin(3.141626*float(j)/5+3.141626*float(i)/10.0f)));
		}
		audio_buffer[i]=b;//a/500;//b;
		//a+=fabs(b);//b*(sin(3.141626*float(i)/400.0f));
	}
	a/=500;
	if(a>1)a=1;
	if(a<0)a=0;
	audio_level=a*a;//audio_level*0.5+0.5*float(a/2);20
    // you could work with stream here...
}
int main( int argc, char *argv[] )
{
    //Quit flag
    bool quit = false;

    //Initialize
    if( init() == false )
    {
        return 1;
    }


	Mix_Music *music = Mix_LoadMUS("2.ogg"); 
	if(music == NULL) 
	{
		printf("Unable to load Ogg file: %s\n", Mix_GetError()); 
		while(1);;
	}
	//if(Mix_PlayMusic(music, -1) == -1) { printf("Unable to play Ogg file: %s\n", Mix_GetError()); return 1; }
	if(Mix_FadeInMusic(music, -1,1000) == -1)
	{
		printf("Unable to play Ogg file: %s\n", Mix_GetError()); 
		while(1);;
	}
	//Mix_HookMusicFinished(musicFinished);

	/*
	Sleep(2000);
	Mix_PauseMusic();
	mciSendString("open \"1.mp3\" type MPEGVideo alias bgmusic", NULL, 0, 0); 
	mciSendString("play bgmusic notify repeat", NULL, 0, 0);
	mciSendString("setaudio bgmusic volume to 1000", 0, 0, 0);
	Sleep(2000);
	Mix_ResumeMusic();
	*/

	//Mix_Music *music2 = Mix_LoadMUS("1.ogg"); 
	//Mix_PlayMusic(music2, -1);

    //The frame rate regulator
    Timer fps;

	//Wait for user exit
	while( quit == false )
	{
        //Start the frame timer
        fps.start();

        //While there are events to handle
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
			{
                quit = true;
            }
            else if( event.type == SDL_KEYDOWN )
            {
                //Handle keypress with current mouse position
                int x = 0, y = 0;
                SDL_GetMouseState( &x, &y );
                handleKeys( event.key.keysym.unicode, x, y );
            }
		}

        //Run frame update
        update();

        //Render frame
        render();

        //Cap the frame rate
        if( fps.get_ticks() < 1000 / FRAMES_PER_SECOND )
        {
            SDL_Delay( ( 1000 / FRAMES_PER_SECOND ) - fps.get_ticks() );
        }
	}

	//Clean up
	clean_up();

	return 0;
}
