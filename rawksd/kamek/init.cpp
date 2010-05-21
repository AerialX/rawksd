#include "rb2.h"
#include <files.h>

extern "C" void Initialise(App* app)
{
	OSReport("RawkSD Haxx Commence!\n");
	OSReport("File_Init(): %d\n", File_Init());
	app->Run();
}

