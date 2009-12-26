#pragma once

#include <proxiios.h>

namespace ProxiIOS { namespace DIP {
	struct Shift
	{
		u32 Size;
		u32 OriginalOffset;
		u32 Offset;
	};
	struct FileDesc
	{
		union {
			u32 Cluster;
			char* Filename;
		};
	};
	struct Patch
	{
		s16 File;
		u32 Offset;
		u32 Length;
	};
} }
