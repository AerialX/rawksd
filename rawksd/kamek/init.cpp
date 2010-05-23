#include "rb2.h"
#include <files.h>

extern "C" void Initialise(App* app)
{
	File_Init();
	app->Run();
}

