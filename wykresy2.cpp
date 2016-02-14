 #define WIN32_LEAN_AND_MEAN // nie includuje niepotrzebnych libow windowsa
#include <cmath>
#include <ctime>
#include <gl\glaux.h>

bool fullscreen=true;
const int SZER = 1920;
const int WYS = 1080;			//rozdzielczosc ekranu
float x, y, z, t = 0;
const float piprzez180 = 3.14159 / 180;

HDC		hDC=NULL;			// Private GDI Device Context
HGLRC	hRC=NULL;			// Permanent Rendering Context
HWND	hWnd=NULL;			// Holds Our Window Handle

double mouse_x, mouse_y;
double myszX, myszY;
int mouse_wrap_x = 0;
int mouse_wrap_y = 0;
double czulosc = 4;			// czulosc myszki

bool klawisze[256];			// tablica trzymajaca kolejke wcisnietych klawiszy
bool active=TRUE;			// okno domyslnie aktywne

double pozX = 1;					// poz x poczatkowa
double pozZ = 5;					// poz z poczatkowa
double pozY = 1;					// poz y poczatkowa
double xtra = 0;					// poz -x
double ztra = 0;					// poz -z
double XP=0;						// przyspieszenie gracza w kierunku osi X
double ZP=0;						// przyspieszenie gracza w kierunku osi Z
double sc_obrY;						// kat obrotu sceny
double obrot;						// kat obrotu gracza
double _heading = 0;
double zprot;						// biezaca predkosc obrotu klawiszami kursora

bool done=FALSE;					// jesli 1 to wyjscie z programu

void przeskalujScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}
	
	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	glLoadIdentity();									// Reset The Modelview Matrix
	gluPerspective(60.0f, (GLdouble)width / (GLdouble)height, 0.001f, 2250.0f);	 //ostatni param. to wielkosc sceny

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}
bool inicjujGL()													// All Setup For OpenGL Goes Here
{
	LPARAM lParam=0;
	SetCursorPos(322,340);						//poczatkowa pozycja celownika
	float temp_mouse_x = LOWORD(lParam);
	float temp_mouse_y = HIWORD(lParam);
	SetCursorPos(320,340);
	return TRUE;										//wszystko ok
}
bool renderujScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);	//czyszcznie sceny i bufora glebi

	XP *= 0.9f;				//zmniejszenie aktualnego przyspieszenia gracza do zera
	ZP *= 0.9f; 
	pozX += XP*.01;		//do aktualnego przesuniecia sceny dodajemy aktualne przyspieszenie
	pozZ += ZP*.01;
	xtra = -pozX;		//przesuniecie sceny w keirunku przeciwnym do ruchu gracza (symulacja ruchu)
	ztra = -pozZ;
	
	zprot*=.9f;				//zmnieszenie aktualnego przyspieszenia katowego do zera (obracanie klawiszami kursora)
	_heading += zprot;		//do aktualnego obrotu sceny w poziomie dodanie przyspieszenia katowego
	obrot = myszX + _heading;	//uwzglednienie poleznia myszki w poziomie
	sc_obrY = 360.0f - obrot;		//obrot sceny w przeciwnym kierunku niz obrot gracz (symulacja obrotu)

	glLoadIdentity();

	glRotated(myszY,1.f,0,0);				//obrot kata kamery gora-dol
	glRotated(sc_obrY,0,1.f,0);				//obrot kamery prawo-lewo
	glTranslated(xtra, -pozY, ztra);		//przesuniecie kemery x, y, z


	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);			//osie ukladu wspolrzednych
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(2200.0f, 0.0f, 0.0f);
		glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 2200.0f, 0.0f);
		glColor4f(0.0f, 0.0f, 1.0f, 1.0f);			
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 2200.0f);
	glEnd();
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);			
	
	t+=0.02;
	for (z = -10; z < 10; z += .2)
	{
		glBegin(GL_LINE_STRIP);
		for ( x = -10; x < 10; x += .01)
		{
			//y = tan(x);
			//y = abs(x)-abs(z);
			//y = sin(x)*tan(z)*sin(t);
			//y = exp(-x*x-z*z)*sin(t);
			y = x*x*x*x+2*x*x*x-2*x*x*sin(t)-x+z*z;
			//y = sin(x)*sin(z)*sin(t);
			//y = 1/x;
			//y = (x*x + z*z) *sin(t);
			//y = x-z;
			glVertex3f(x, y, z);
		}
		glEnd();
	}

	
	return TRUE;					// kontynuuj renderowanie nastepnej klatki
}
void zamknijOkno()										// zamkniecie okna
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}
}
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	 
	if (uMsg == WM_MOUSEMOVE)
	{
		if (LOWORD(lParam) > 370) 
			SetCursorPos(10,(int)mouse_y), mouse_wrap_y--;
		else if (LOWORD(lParam) < 10) 
			SetCursorPos(370,(int)mouse_y), mouse_wrap_y++;

		if (HIWORD(lParam) > 370) 
			SetCursorPos((int)mouse_x,10), mouse_wrap_x--;
		else if (HIWORD(lParam) < 10) 
			SetCursorPos((int)mouse_x,370), mouse_wrap_x++;

		if (abs(mouse_x-LOWORD(lParam)) > 300)
		{
			if (mouse_x > 360/2)
				myszX += ((mouse_x-360)-LOWORD(lParam))/czulosc;
			else if (LOWORD(lParam) > 360/2)
				myszX += (mouse_x-(LOWORD(lParam)-360))/czulosc;
		}
		else
		{ 
			myszX += (mouse_x-LOWORD(lParam))/czulosc;
		} 

		if (myszX > 360) myszX = 0;
		else if (myszX < 0) myszX = 360;

		if (abs(mouse_y-HIWORD(lParam)) > 300)
		{ 
			if (mouse_y > 360/2)
				myszY -= ((mouse_y-360)-HIWORD(lParam))/czulosc;
			else if (LOWORD(lParam) > 360/2)
				myszY -= (mouse_y-(HIWORD(lParam)-360))/czulosc;
		}
		else
		{ 
			myszY -= (mouse_y-HIWORD(lParam))/czulosc;
		} 

		if (myszY > 85) myszY = 85;
		else if (myszY < -85) myszY = -85;

		mouse_x = LOWORD(lParam);          
		mouse_y = HIWORD(lParam);
	}			

	else if (uMsg == WM_LBUTTONDOWN)
	{     


	}
	else if (uMsg == WM_ACTIVATE)							// Watch For Window Activate Message
	{
		if (!HIWORD(wParam))					// Check Minimization State
		{
			active=TRUE;						// Program Is Active
		}
		else
		{
			active=FALSE;						// Program Is No Longer Active
		}

	}

	else if (uMsg == WM_SYSCOMMAND)							// Intercept System Commands
	{
		switch (wParam)							// Check System Calls
		{
		case SC_SCREENSAVE:					// Screensaver Trying To Start?
		case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?

			return 0;							// Prevent From Happening
		}
	}

	else if (uMsg == WM_CLOSE)								// Did We Receive A Close Message?
	{
		PostQuitMessage(0);						// Send A Quit Message
	}

	else if (uMsg == WM_KEYDOWN)							// Is A Key Being Held Down?
	{
		klawisze[wParam] = TRUE;					// If So, Mark It As TRUE
	}

	else if (uMsg == WM_KEYUP)								// Has A Key Been Released?
	{
		klawisze[wParam] = FALSE;					// If So, Mark It As FALSE
	}

	else if (uMsg == WM_SIZE)								// Resize The OpenGL Window
	{
		przeskalujScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
	}        		

	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	HINSTANCE	hInstance;				// Holds The Instance Of The Application
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style


	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	ChangeDisplaySettings(NULL,0);								// If So Switch Back To The Desktop

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}

	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;


		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		//width = 1920;
		//height = 1080;
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height

		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			//width = 1366;
			//height = 768;
			dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
			dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height

			if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
			{

				// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
				if (MessageBox(NULL,"Tryb pelnoekranowy nie jest dostepny. Uruchomic w oknie?","GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
				{
					fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
				}
				else
				{
					// Pop Up A Message Box Letting User Know The Program Is Closing.
					MessageBox(NULL,"Program zamkniety.","ERROR",MB_OK|MB_ICONSTOP);
					return FALSE;									// Return FALSE
				}
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;						// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,			// Extended Style For The Window
		"OpenGL",			// Class Name
		title,				// Window Title
		dwStyle,			// Window Style
		0, 0,				// Window Position
		width, height,		// Selected Width And Height
		NULL,				// No Parent Window
		NULL,				// No Menu
		hInstance,			// Instance
		NULL)))				// Dont Pass Anything To WM_CREATE
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	przeskalujScene(width, height);					// Set Up Our Perspective GL Screen

	if (!inicjujGL())								// Initialize Our Newly Created GL Window
	{
		zamknijOkno();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure

	if (!CreateGLWindow("Wykresy funkcji 2 zmiennych w FPS :)",SZER,WYS,32,fullscreen))
	{
		return 0;
	}

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From renderujScene()
			if ((active && !renderujScene()) || klawisze[VK_ESCAPE])	// Active?  Was There A Quit Received?
			{
				done=TRUE;							// ESC or renderujScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

				if (klawisze[VK_UP])  // Move forwards
				{
					XP -= (GLdouble)sin(obrot*piprzez180);	
					ZP -= (GLdouble)cos(obrot*piprzez180);
				}
				else if (klawisze['W'])  // Move forwards
				{
					XP -= (GLdouble)sin(obrot*piprzez180);	
					ZP -= (GLdouble)cos(obrot*piprzez180);
				}

				if (klawisze[VK_DOWN]) // Move backwards
				{
					XP += (GLdouble)sin(obrot*piprzez180);	
					ZP += (GLdouble)cos(obrot*piprzez180);
				}	               
				else if (klawisze['S']) // Move backwards
				{
					XP += (GLdouble)sin(obrot*piprzez180);	
					ZP += (GLdouble)cos(obrot*piprzez180);
				}	

				if (klawisze['A'])  // strafe left
				{
					XP += (GLdouble)sin((obrot-90)*piprzez180);	
					ZP += (GLdouble)cos((obrot-90)*piprzez180);
				}

				if (klawisze['D']) // strafe right
				{
					XP += (GLdouble)sin((obrot+90)*piprzez180);	
					ZP += (GLdouble)cos((obrot+90)*piprzez180);
				}	               

				if (klawisze['Q']) // na dol
				{
					pozY-=.1;
				}	               

				if (klawisze['E']) // do gory
				{
					pozY+=.1;
				}	               

				if (klawisze['-'])
				{
					czulosc-=0.1;
				}	               

				if (klawisze['='])
				{
					czulosc+=0.1;
				}	               


				if (klawisze[VK_SPACE])
				{
				}

				if (klawisze[VK_LEFT]) // Turn left
				{
					zprot += .1f;
				}
				else if (klawisze[VK_RIGHT]) // Turn right
				{
					zprot -= .1f;
				}			

			}
		}
	}

	zamknijOkno();
	return (msg.wParam);						// wyjscie z programu
}
