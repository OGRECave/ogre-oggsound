#include <iostream>
#include <string>
#include <windows.h>
#include "Ogre.h"
#include "MainOgre.h"
#include "InputManager.h"
#include "MainApp.h"

#include <stdio.h>
#include <fcntl.h>
#include <io.h>


#ifdef _STEREO
#include "StereoCamera.h"
#endif

void showWin32Console();

MainOgre *og = NULL;

//-----------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg)
	{
		HDC   hdc;
		PAINTSTRUCT ps;			

		case WM_SIZE:			
			if (og)
			{			
				MainApp::getSingleton().resize();				
				break;
			}		

		case WM_PAINT:				
			hdc = BeginPaint(hWnd,&ps); 			
			EndPaint(hWnd,&ps); 
			break;				

		case WM_DESTROY:		
			PostQuitMessage(0);
			return 0;					

		default:
			break;
	} 

	return DefWindowProc( hWnd, msg, wParam, lParam );
}
//-----------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
// First simple memory leak detection for VS2005/VS2008
#if (defined( WIN32 ) || defined( _WIN32 )) && defined( _MSC_VER ) && defined( _DEBUG ) 
   // use _crtBreakAlloc to put a breakpoint on the provided memory leak id allocation
   //_crtBreakAlloc = 62144;     
   _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif 
   
   // Register the mWindow class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
		GetModuleHandle(NULL), NULL, LoadCursor(NULL,IDC_ARROW), NULL, NULL,
		"Ogre Win32", NULL };
	 
	RegisterClassEx( &wc );


// Create the application's window full screen STEREO      
#ifdef _STEREO
	HWND hWnd = CreateWindow( "Ogre Win32", "OgreAudiere", 
                               WS_POPUP|WS_VISIBLE  , 0, 0, 1360, 768,
                              0, NULL, wc.hInstance, NULL );  
#else
// Create the application's window full screen MONO
//in a window
	HWND hWnd = CreateWindow( "Ogre Win32", "Sample 1", 
                              WS_OVERLAPPEDWINDOW  , 0, 0, 800, 600,
                              0, NULL, wc.hInstance, NULL );
//full screen
	// Create the application's window full screen MONO
/*	HWND hWnd = CreateWindow( "Ogre Win32", "OgreAudiere", 
                              WS_POPUP|WS_VISIBLE  , 0, 0, 1366, 768,
                              0, NULL, wc.hInstance, NULL );*/
#endif

	showWin32Console();	

	//og = MainOgre::createOgre(hWnd); 	
	og = new MainOgre(hWnd);

	og->Init();

	MainApp *app = new MainApp;	

	ShowWindow( hWnd, SW_SHOWDEFAULT );		
	UpdateWindow( hWnd );

	MSG msg;
	ZeroMemory(&msg,sizeof(msg)); 
	while(msg.message != WM_QUIT)
	{	
		if (PeekMessage( &msg,NULL,0,0,PM_REMOVE ))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		else 
		{			
			if (!og->Render())
				break;
		}		
	}
	
	delete app;	
	delete og;

	ChangeDisplaySettings(0, 0);

	return 0;
}
/*-----------------------------------------------------------------------*/
void showWin32Console()
{
	static const WORD MAX_CONSOLE_LINES = 5000;
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;
	// allocate a console for this app
	AllocConsole();
	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
	coninfo.dwSize);
	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );
	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );
	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );
	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	std::ios::sync_with_stdio();
}