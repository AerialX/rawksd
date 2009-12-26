#pragma once

#include "gctypes.h"

#include "gcutil.h"

inline u32 cntlzw(u32 num)
{
	// count leading zero bits... oh god the lack of asm :o
	u32 ret = 0;
	while (!(num & 1)) {
		ret++;
		num >>= 1;
	}
	return ret;
}

#define _CPU_ISR_Enable(...)
#define _CPU_ISR_Disable(...)
#define _CPU_ISR_Restore( _isr_cookie )
#define _CPU_ISR_Flash( _isr_cookie )
#define _CPU_FPR_Enable()
#define _CPU_FPR_Disable()
