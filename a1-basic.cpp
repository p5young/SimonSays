//
// CS349
// Fall '17
// Assignment 1
// 20491854
// 
// Simon game
// 
//

#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include <sys/time.h>
#include <sstream> 	// for int to string converter
#include <string>	// for displaying text
#include <math.h> 	// sqrt, pow, and sin

// Header files for X functions
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <unistd.h> // needed for sleep

#include "simon.h" // game object

using namespace std;

// helper to convert int to string
template < typename T > string to_string( const T& n )	{
    std::ostringstream stm;	// declare oss
    stm << n;				// initialize oss as number to be converted
    return stm.str();		// return
} // to_string

// get microseconds
unsigned long now() {		// routine written by prof, used for getting time
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

// information to draw on the window
struct XInfo {
  Display* display;			// the display to print to
  Window window;			// window containing the display
  int screen;				// which screen we're using
  GC gc;					// graphics context
}; // XInfo

XInfo xinfo;				// forward declaration

int sinwave = 0;	// counter for sinewave movement
int depth;			// depth of the display
Pixmap buffer;		// buffer for double buffer use
bool wiggle = true;	// flag for making buttons siggle in a sinwave pattern
int FPS = 60;		// frames per second to run animation loop

// helper function to set X foreground colour
enum Colour {BLACK, WHITE};
void setForeground(Colour c) {
	if (c == BLACK) {
		XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, xinfo.screen));
	} else {
		XSetForeground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));
	}
}


// Abstract class for displayable things
class Displayable {
	public:
		virtual void paint(XInfo& xinfo) = 0;
}; // Displayable

// Text displayable object
class Text : public Displayable {
	public:
		// Paint routine draws the text to the buffer
		virtual void paint(XInfo& xinfo) {
			setForeground(BLACK);
	    	XDrawImageString( xinfo.display, buffer, xinfo.gc,
	                      	this->x, this->y, this->s.c_str(), this->s.length() );
		} // paint

		// alter the message displayed
		void changetext(string input){
			s = input;		// change the string stored in this text object
		}

		// constructor
		Text(int x, int y, string s): x(x), y(y), s(s)  {}

	private:
		int x;		// x coordinate
		int y;		// y coordinate
		string s;	// string to show
}; // Text

// Button displayable object
class Button : public Displayable {
	public:
		// Paint this button to buffer
		virtual void paint(XInfo& xinfo) {
			if (wiggle) {
				wy = y + 10 * sin((sinwave + (inum * 17)) / 10);	// wiggle function
			} else { 
				wy = y;												// wy = y means button sits still
			} // if

			if (clicked) {											// true if I am doing a clicked animation
				unsigned long end = now();
				if (now() - lastclicked < (30000000 / FPS)) {		// true if I was clicked recently
					dd -= 3;
					setForeground(BLACK);
    				XSetFillStyle(xinfo.display,  xinfo.gc, FillSolid);
    				XFillArc(xinfo.display, buffer, xinfo.gc,
           						x - (diameter / 2), wy - (diameter / 2), diameter, diameter, 0, 360 * 64);
    		    	setForeground(WHITE);
    		        XDrawArc(xinfo.display, buffer, xinfo.gc,
               				x - (dd / 2), wy - (dd / 2), dd, dd, 0, 360 * 64);
    		        dd -= 3;
				} else {					// executes if I've exceeded by clicked animation time
					clicked = false;		// reset clicked flag
					dd = diameter;			// reset animation ring diameter
					circlenumber();			// print out my standard appearance to buffer
				} // if
			} else if (mouseover){									// true if there's a mouse inside my radius
				XSetLineAttributes(xinfo.display, xinfo.gc, 4,		// change line width to 4
									LineSolid, CapButt, JoinRound);
				circlenumber();										// print out my appearance to buffer
				XSetLineAttributes(xinfo.display, xinfo.gc, 1,		// change line width to 1
									LineSolid, CapButt, JoinRound);
			} else {
				circlenumber();										// no special flags, print my standard appearance
			} // if
		} // paint

		// changes the x & y coordinates of this button
		void move(int _x, int _y) {
			x = _x;
			y = _y;
		} // move

		// sets the mouseover flag to true if there's a pointer within my radius
		// false otherwise
		void mousemove(int _x, int _y) {
			float dist = sqrt(pow(_x - x, 2) + pow(_y - wy, 2));	// distance formula uses wy in case I'm wiggling
			if (dist < diameter / 2) {	// pointer in radius
				mouseover = true;
			} else {					// pointer not in radius
				mouseover = false;
			} // if
		} // mousemove

		// click which checks against x/y coordinates
		// returns true if click is in my radius
		// false otherwise
		bool click(int _x, int _y) {
			float dist = sqrt(pow(_x - x, 2) + pow(_y - wy, 2));
			if (dist < diameter / 2) return click();
			else return false;
		}

		// click which clicks the button regardless of mouse location
		// used mainly by simon.h module to click this button
		bool click() {
			clicked = true;			// change clicked flag to true
			lastclicked = now();	// set not as the time last clicked
			dd = diameter;			// reset the ring animation diameter
			return true;			// returns true, to be passed along by click(x,y) if necessary
		}

		// constructor
		Button(int x, int y, int num): x(x), y(y), inum(num) {
			diameter = 100;			// initialize diameter
			mouseover = false;		// initialize mouseover flag
			clicked = false;		// initialize clicked flag
			snum = to_string(num);	// initialize snum, or string-num, this button number
			wy = y;					// initialize wy, or wiggly-y, my moving y coordinate
		}

	private:
		// geometry
		int x;
		int y;
		int diameter;
		// indicates if a mouse is hovering over the button
		bool mouseover;
		// wiggle y is the y coordinate when wiggling
		int wy;
		// button number
		int inum;		// as int
		string snum;	// as string
		// variables for clicked animation
		bool clicked;				// indicates this button has been clicked
		unsigned long lastclicked;	// time when the button was last clicked
		int dd;						// the ring diameter in the clicked animation

		// Draw the button using a circular arc and put the button number in the middle
		void circlenumber(){
			setForeground(BLACK);																// make foreground black
		    XSetBackground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));	// make background white
			XDrawArc(xinfo.display, buffer, xinfo.gc,											// Draw circle for button
			        x - diameter / 2,
			        wy - diameter / 2,
			        diameter, diameter,
			        0, 360 * 64);
			XDrawImageString(xinfo.display, buffer, xinfo.gc,									// Draw number for button
                      			this->x - 6, this->wy + 10, this->snum.c_str(), 1 );
		}
}; // Button


// function to put error message on screen
void error( string str ) {
  cerr << str << endl;
  exit(0);
}


// create a window and initialize XInfo structure
// code used from examples in class
void initX(int argc, char* argv[]) {

  /*
  * Display opening uses the DISPLAY  environment variable.
  * It can go wrong if DISPLAY isn't set, or you don't have permission.
  */
  xinfo.display = XOpenDisplay( "" );
  if ( !xinfo.display ) {
    error( "Can't open display." );
  }

  /*
  * Find out some things about the display you're using.
  */
  // DefaultScreen is as macro to get default screen index
  xinfo.screen = DefaultScreen( xinfo.display );

  unsigned long white, black;
  white = XWhitePixel( xinfo.display, xinfo.screen );
  black = XBlackPixel( xinfo.display, xinfo.screen );

  xinfo.window = XCreateSimpleWindow(
                   xinfo.display,       // display where window appears
                   DefaultRootWindow( xinfo.display ), // window's parent in window tree
                   0, 0,                  // upper left corner location
                   800, 400,                  // size of the window
                   0,               // width of window's border
                   black,           // window border colour
                   white );             // window background colour

  // extra window properties like a window title
  XSetStandardProperties(
    xinfo.display,  // display containing the window
    xinfo.window,   // window whose properties are set
    "a1",  			// window's title
    "a1",			// icon's title
    None,       	// pixmap for the icon
    argv, argc,     // applications command line args
    None );         // size hints for the window

  // Tell the window manager what input events you want.
    XSelectInput( xinfo.display, xinfo.window,
                  ButtonPressMask | KeyPressMask | 
                  ExposureMask | StructureNotifyMask | 
                  PointerMotionMask );

  /*
   * Put the window on the screen.
   */
  XMapRaised( xinfo.display, xinfo.window );

  XFlush(xinfo.display);

  // create a simple graphics context
	xinfo.gc = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	setForeground(BLACK);//XSetForeground(xinfo.display, xinfo.gc, black);
	setForeground(WHITE);//XSetBackground(xinfo.display, xinfo.gc, white);

	// load a larger font
	XFontStruct * font;
	font = XLoadQueryFont (xinfo.display, "12x24");
	XSetFont (xinfo.display, xinfo.gc, font->fid);

  // give server time to setup
  sleep(1);
}

/*
 * Function to repaint a display list
 */
void repaint( list<Displayable*> dList, XInfo& xinfo, XWindowAttributes &w) {
  list<Displayable*>::const_iterator begin = dList.begin();
  list<Displayable*>::const_iterator end = dList.end();

  if (sinwave == 3600) sinwave = 0;
  else ++sinwave;

	setForeground(WHITE);//XSetForeground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, DefaultScreen(xinfo.display)));
	XFillRectangle(xinfo.display, buffer, xinfo.gc,
	               0, 0, w.width, w.height);

  //XClearWindow(xinfo.display, xinfo.window);
  while ( begin != end ) {
    Displayable* d = *begin;
    d->paint(xinfo);
    begin++;
  }

  				XCopyArea(xinfo.display, buffer, xinfo.window, xinfo.gc,
				          0, 0, w.width, w.height,  // region of pixmap to copy
				          0, 0); // position to put top left corner of pixmap in window
  XFlush(xinfo.display);
}

// Loops until an event makes it return
// or until it has refreshed for 'counter' number of frames
// counter to be set to -1 if you want to wait for event indefinitely
//
// returns button number, if a button is clicked
// returns -1 if 'q' is pressed and you wish to exit the program
// returns -2 if spacebar is pressed or counter limit reached
int eventloop(XWindowAttributes &w, int n, vector<Button*> buttons, list<Displayable*> dList, int counter) {

	XEvent event;	// event to check type against
	KeySym key;		// key
	char text[100];	// text buffer for input

	// Used with counter to only run loop for a certain number of frames
	// If counter == -1, loop will only exit after handling an event
	int numloops = 0;

	// time of last xinfo.window paint
	unsigned long lastRepaint = 0;

	int buttonspace;

	while (true) {

		if (XPending(xinfo.display)) {				// Only check for event type if an event is pending
			XNextEvent( xinfo.display, &event );

	        switch ( event.type ) {

	        case ConfigureNotify:										// window has resized
	        	XGetWindowAttributes(xinfo.display, xinfo.window, &w);	// check new size
	        	buttonspace = w.width / (n+1);							// recalculate button spacing
	        	for (int i = 1; i <= n ; ++i){							// loop over all buttons
	        		buttons[i-1]->move(buttonspace * i, w.height/2);	// assign buttons new xy coordinates
	        	}
	        	break;

	        case MotionNotify:													// mouse has moved
	        	for (int i = 0 ; i < n ; ++i){									// for all buttons
	        		buttons[i]->mousemove(event.xbutton.x, event.xbutton.y);	// check for mouseover
	        	}
	        	break;

	        case ButtonPress:													// mouse clicked
				for (int i = 0 ; i < n ; ++i){
	        		if (buttons[i]->click(event.xbutton.x, event.xbutton.y)){	// check if button clicked
	        			return i;												// return which button was pushed
	        		}
	        	}
	        	break;

	        // Exit when q is pressed or start game when space is pressed
	        case KeyPress:
	        	int i = XLookupString((XKeyEvent*)&event, text, 100, &key, 0 );
	            if ( i == 1 && text[0] == 'q' ) {
	                cout << "Terminated normally." << endl;
	                XCloseDisplay(xinfo.display);
	                return -1;			// -1 is return code for exit program
	            }
	            if ( i == 1 && text[0] == ' ' ) {
	            	return -2;			// -2 is return code for spacebar pressed
	            }
	            break;
	        } // switch
    	} // if

    	//check current time
        unsigned long end = now();

        // repaint all displayables
		if (end - lastRepaint > 1000000 / FPS) {	// check time since last repaint

			numloops++;								// increment number of frames printed

			if (numloops == counter) return -2; 	// exit if frame limit reached

			repaint( dList, xinfo, w);				// repaint all displayables

			lastRepaint = now(); 					// remember when the paint happened

		} // repainter
    } // while loop
} // eventloop()

int main( int argc, char* argv[] ) {

	// get the number of buttons from args
	int n = 4;							// default value
    if (argc > 1) n = atoi(argv[1]);	// n specified by command line
    if (n > 6) n = 6;					// max of 6 buttons
    if (n < 1) n = 1;					// min of 1 button

    // create the Simon game object
	Simon simon = Simon(n, true);

	// initialize
	initX(argc, argv);

	// return code used by event loop
	int retval;

	// get window attributes
	XWindowAttributes w;
	XGetWindowAttributes(xinfo.display, xinfo.window, &w);

	depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
	buffer = XCreatePixmap(xinfo.display, xinfo.window, w.width, w.height, depth);

	// list of Displayables
	list<Displayable*> dList;

	// populate list of Displayables
	Text* score = new Text(50, 50, to_string(simon.getScore()));

	Text* banner = new Text(50, 100, "Press SPACE to play.");

	dList.push_back(score);		// scoreboard, top left

	dList.push_back(banner);	// message board, just below scoreboard

	int buttonspace = w.width / (n+1);	// calculate gap between buttons as window width / number of buttons + 1

	vector<Button*> buttons;			// vector of buttons

	// initialize vector of buttons
	for (int i = 1; i <= n ; ++i){
			buttons.push_back(new Button(buttonspace * i, w.height/2, i));
			dList.push_back(buttons[i-1]);
	}

	// initialize visual display
	repaint( dList, xinfo, w);

	// gameloop
	while (true) {

		// make buttons wiggle
		wiggle = true;

		switch (simon.getState()) {

		// will only be in this state right after Simon object is contructed
		case Simon::START:
			banner->changetext("Press SPACE to play.");
			break;
		// they won last round
		// score is increased by 1, sequence length is increased by 1
		case Simon::WIN:
			banner->changetext("You won! Press SPACE to continue.");
			break;
		// they lost last round
		// score is reset to 0, sequence length is reset to 1
		case Simon::LOSE:
			banner->changetext("You lose. Press SPACE to play again.");
			break;
		default:
			// should never be any other state at this point ...
			break;
		}

		while(true){
			retval = eventloop(w,n,buttons,dList,-1);
			if (retval == -2) break;
			else if (retval == -1) return 0;
		}

		// stop button wiggling
		wiggle = false;

		// start new round with a new sequence
		simon.newRound();

		// computer plays
		banner->changetext("Watch what I do...");					// change message

		// continue while state is COMPUTER
		while (simon.getState() == Simon::COMPUTER) {				// loop while computer's turn
			if (eventloop(w,n,buttons,dList,45) == -1) return 0;	// do the event loop for 45 frames (3/4 of a second)
			buttons[simon.nextButton()]->click();					// click the button the simon game picks next
		}

		// now human plays
		banner->changetext("Your turn ...");						// change message
		while (simon.getState() == Simon::HUMAN) {					// loop while human's turn
			retval = eventloop(w,n,buttons,dList,-1);				// run event loop until an event returns a value
			if (retval == -1) return 0;								// exit game if 'q' is pressed
			if (!simon.verifyButton(retval)) {						// otherwise check if the button pressed was correct
				cout << "wrong" << endl;
			}
		}

		// update scoreboard
		score->changetext(to_string(simon.getScore()));				// update scoreboard before next round

	} // while, gameloop
} // main