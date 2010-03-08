#pragma once

#include <proxiios.h>

namespace ProxiIOS { namespace DIP {
	struct OffsetPatch
	{
		u32 Offset;
		u32 Length;
	};
	
	struct Patch : public OffsetPatch
	{
		u16 File;
	};
	
	struct Shift : public OffsetPatch
	{
		u32 OriginalOffset;
	};
	
	struct FileDesc
	{
		union {
			u32 Cluster;
			char* Filename;
		};
	};
} }
