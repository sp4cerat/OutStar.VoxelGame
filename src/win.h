#include <Commdlg.h>

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms646839(v=vs.85).aspx

int refresh_thread_state=0;

DWORD WINAPI refresh_thread( LPVOID lpParam )
{
	while(1)
	{
		SDL_GL_SwapBuffers();
		Sleep(20);
		if(refresh_thread_state==1) break;
	}
	refresh_thread_state=2;

	return 0;
}

void update_bg_begin()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	glViewport(0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
	glDisable(GL_BLEND);
	glReadBuffer (GL_FRONT);
	glDrawBuffer (GL_BACK);
	glRasterPos2i(0,0);
	glCopyPixels (0,0,WINDOW_WIDTH,WINDOW_HEIGHT,GL_COLOR);
	glFlush();
	CreateThread( 0,0,refresh_thread,0,0,0);
}

void update_bg_end()
{
	refresh_thread_state=1;
	while(refresh_thread_state!=2) Sleep(100);
	refresh_thread_state=0;
}


char* win_getopenfilename(char* filter="All Files(*.*)\0*.*\0", uint flags=OFN_ENABLESIZING|OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY)
{
	char szTmp[1000] = "";
	GetCurrentDirectory( 1000, szTmp );

	OPENFILENAME of;
	static char szFileName[1000] = "";

	ZeroMemory(&of, sizeof(of));
	of.lpstrTitle = "Load File";
	of.lStructSize = sizeof(of);
	of.hwndOwner = WINDOW_HWND;
	of.lpstrFilter = filter;
	of.lpstrFile = szFileName;
	of.nMaxFile = 1000;
	of.Flags = flags;
	of.lpstrDefExt = "*.*";
	of.lpstrInitialDir = EXE_DIR;
	
	update_bg_begin();

	int res= GetOpenFileName(&of);

	update_bg_end();

	SetCurrentDirectory( szTmp );

	return (res) ? szFileName : 0;
}

char* win_getsavefilename(char* filter="All Files(*.*)\0*.*\0", uint flags=OFN_EXPLORER | OFN_HIDEREADONLY|OFN_ENABLESIZING)
{
	char szTmp[1000] = "";
	GetCurrentDirectory( 1000, szTmp );

	OPENFILENAME of;
	static char szFileName[1000] = "";

	ZeroMemory(&of, sizeof(of));

	of.lStructSize = sizeof(of);
	of.lpstrTitle = "Save File";
	of.lpstrFilter = filter;
	of.lpstrFile = szFileName;
	of.nMaxFile = 1000;
	of.Flags = flags;
	of.lpstrDefExt = "*.*";
	of.lpstrInitialDir = EXE_DIR;
	of.hwndOwner = WINDOW_HWND;

	update_bg_begin();

	int res= GetSaveFileName(&of);

	update_bg_end();

	SetCurrentDirectory( szTmp );

	return (res) ? szFileName : 0;
}

bool win_messagebox_confirm(char* title,char* text)
{
	int a=MessageBox(WINDOW_HWND,text,title,MB_OKCANCEL|MB_ICONINFORMATION| MB_TOPMOST);
	return (a==IDOK);
}
