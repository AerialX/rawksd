/*-------------------------------------------------------------

dvd.h -- DVD subsystem

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/


#ifndef __DVD_H__
#define __DVD_H__

/*! 
 * \file dvd.h 
 * \brief DVD subsystem
 *
 */ 

#include <gctypes.h>

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


/*!
 * \typedef struct _dvddiskid dvddiskid
 * \brief forward typedef for struct _dvddiskid
 */
typedef struct _dvddiskid dvddiskid;

/*!
 * \typedef struct _dvddiskid dvddiskid
 *
 *        This structure holds the game vendors copyright informations.<br>
 *        Additionally it holds certain parameters for audiocontrol and<br>
 *        multidisc support.
 *
 * \param gamename[4] vendors game key
 * \param company[2] vendors company key
 * \param disknum number of disc when multidisc support is used.
 * \param gamever version of game
 * \param streaming flag to control audio streaming
 * \param streambufsize size of buffer used for audio streaming
 * \param pad[22] padding 
 */
struct _dvddiskid {
	s8 gamename[4];
	s8 company[2];
	u8 disknum;
	u8 gamever;
	u8 streaming;
	u8 streambufsize;
	u8 pad[22];
};

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
