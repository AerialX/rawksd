#pragma once

#include "rb2/Main.h"

struct PassiveMessagesPanel
{
	int QueueMessage(const char* message);
};

PassiveMessagesPanel* GetPMPanel();

