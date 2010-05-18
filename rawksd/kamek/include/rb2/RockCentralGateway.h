#pragma once

#include "rb2/Main.h"

struct RockCentralGateway
{
	int SubmitBandScore(Symbol* song, int unknown1, int unknown2, int unknown3, HxGuid* guid, Hmx::Object* object);
	int SubmitPlayerScore(Symbol* song, int instrument, int difficulty, int score, int unknown, int player, Hmx::Object* object);
};

