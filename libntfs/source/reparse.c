/**
 * reparse.c - Processing of reparse points
 *
 *	This module is part of ntfs-3g library, but may also be
 *	integrated in tools running over Linux or Windows
 *
 * Copyright (c) 2008-2009 Jean-Pierre Andre
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#include "types.h"
#include "debug.h"
#include "attrib.h"
#include "inode.h"
#include "dir.h"
#include "volume.h"
#include "mft.h"
#include "index.h"
#include "lcnalloc.h"
#include "logging.h"
#include "misc.h"
#include "reparse.h"

/* the definitions in layout.h are wrong, we use names defined in
  http://msdn.microsoft.com/en-us/library/aa365740(VS.85).aspx
*/

#define IO_REPARSE_TAG_DFS         const_cpu_to_le32(0x8000000A)
#define IO_REPARSE_TAG_DFSR        const_cpu_to_le32(0x80000012)
#define IO_REPARSE_TAG_HSM         const_cpu_to_le32(0xC0000004)
#define IO_REPARSE_TAG_HSM2        const_cpu_to_le32(0x80000006)
#define IO_REPARSE_TAG_MOUNT_POINT const_cpu_to_le32(0xA0000003)
#define IO_REPARSE_TAG_SIS         const_cpu_to_le32(0x80000007)
#define IO_REPARSE_TAG_SYMLINK     const_cpu_to_le32(0xA000000C)

struct MOUNT_POINT_REPARSE_DATA {      /* reparse data for junctions */
	le16	subst_name_offset;
	le16	subst_name_length;
	le16	print_name_offset;
	le16	print_name_length;
	char	path_buffer[0];      /* above data assume this is char array */
} ;

struct SYMLINK_REPARSE_DATA {          /* reparse data for symlinks */
	le16	subst_name_offset;
	le16	subst_name_length;
	le16	print_name_offset;
	le16	print_name_length;
	le32	flags;		     /* 1 for full target, otherwise 0 */
	char	path_buffer[0];      /* above data assume this is char array */
} ;

struct INODE_STACK {
	struct INODE_STACK *previous;
	struct INODE_STACK *next;
	ntfs_inode *ni;
} ;

static const ntfschar dir_junction_head[] = {
	const_cpu_to_le16('\\'),
	const_cpu_to_le16('?'),
	const_cpu_to_le16('?'),
	const_cpu_to_le16('\\')
} ;

static const ntfschar vol_junction_head[] = {
	const_cpu_to_le16('\\'),
	const_cpu_to_le16('?'),
	const_cpu_to_le16('?'),
	const_cpu_to_le16('\\'),
	const_cpu_to_le16('V'),
	const_cpu_to_le16('o'),
	const_cpu_to_le16('l'),
	const_cpu_to_le16('u'),
	const_cpu_to_le16('m'),
	const_cpu_to_le16('e'),
	const_cpu_to_le16('{'),
} ;

static const char mappingdir[] = ".NTFS-3G/";

/*
 *		Fix a file name with doubtful case in some directory index
 *	and return the name with the casing used in directory.
 *
 *	Should only be used to translate paths stored with case insensitivity
 *	(such as directory junctions) when no case conflict is expected.
 *	If there some ambiguity, the name which collates first is returned.
 *
 *	The name is converted to upper case and searched the usual way.
 *	The collation rules for file names are such that we should get the
 *	first candidate if any.
 */

static u64 ntfs_fix_file_name(ntfs_inode *dir_ni, ntfschar *uname,
		int uname_len)
{
	ntfs_volume *vol = dir_ni->vol;
	ntfs_index_context *icx;
	u64 mref;
	le64 lemref;
	int lkup;
	int olderrno;
	int i;
	u32 cpuchar;
	FILE_NAME_ATTR *found;
	struct {
		FILE_NAME_ATTR attr;
		ntfschar file_name[NTFS_MAX_NAME_LEN + 1];
	} find;

	mref = (u64)-1; /* default return (not found) */
	icx = ntfs_index_ctx_get(dir_ni, NTFS_INDEX_I30, 4);
	if (icx) {
		if (uname_len > NTFS_MAX_NAME_LEN)
			uname_len = NTFS_MAX_NAME_LEN;
		find.attr.file_name_length = uname_len;
		for (i=0; i<uname_len; i++) {
			cpuchar = le16_to_cpu(uname[i]);
			/*
			 * We need upper or lower value, whichever is smaller,
			 * here we assume upper is always smaller
			 */
			if (cpuchar < vol->upcase_len)
				find.attr.file_name[i] = vol->upcase[cpuchar];
			else
				find.attr.file_name[i] = uname[i];
		}
		olderrno = errno;
		lkup = ntfs_index_lookup((char*)&find, uname_len, icx);
		if (errno == ENOENT)
			errno = olderrno;
		/*
		 * We generally only get the first matching candidate,
		 * so we still have to check whether this is a real match
		 */
		if (icx->data && icx->data_len) {
			found = (FILE_NAME_ATTR*)icx->data;
			if (lkup
			   && !ntfs_names_collate(find.attr.file_name,
				find.attr.file_name_length,
				found->file_name, found->file_name_length,
				1, IGNORE_CASE,
				vol->upcase, vol->upcase_len))
					lkup = 0;
			if (!lkup) {
				/*
				 * name found :
				 *    fix original name and return inode
				 */
				lemref = *(le64*)((char*)found->file_name
					- sizeof(INDEX_ENTRY_HEADER)
					- sizeof(FILE_NAME_ATTR));
				mref = le64_to_cpu(lemref);
				for (i=0; i<found->file_name_length; i++)
					uname[i] = found->file_name[i];
			}
		}
		ntfs_index_ctx_put(icx);
	}
	return (mref);
}

/*
 *		Search for a directory junction or a symbolic link
 *	along the target path, with target defined as a full absolute path
 *
 *	Returns the path translated to a Linux path
 *		or NULL if the path is not valid
 */

static char *search_absolute(ntfs_volume *vol, ntfschar *path,
				int count, BOOL isdir)
{
	ntfs_inode *ni;
	u64 inum;
	char *target;
	int start;
	int len;

	target = (char*)NULL; /* default return */
	ni = ntfs_inode_open(vol, (MFT_REF)FILE_root);
	if (ni) {
		start = 0;
		do {
			len = 0;
			while (((start + len) < count)
			    && (path[start + len] != const_cpu_to_le16('\\')))
				len++;
			inum = ntfs_fix_file_name(ni, &path[start], len);
			ntfs_inode_close(ni);
			ni = (ntfs_inode*)NULL;
			if (inum != (u64)-1) {
				inum = MREF(inum);
				ni = ntfs_inode_open(vol, inum);
				start += len;
				if (start < count)
					path[start++] = const_cpu_to_le16('/');
			}
		} while (ni
		    && (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)
		    && (start < count));
	if (ni
	    && (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY ? isdir : !isdir))
		if (ntfs_ucstombs(path, count, &target, 0) < 0) {
			if (target) {
				free(target);
				target = (char*)NULL;
			}
		}
	if (ni)
		ntfs_inode_close(ni);
	}
	return (target);
}

/*
 *		Stack the next inode in the path
 *
 *	Returns the new top of stack
 *		or NULL (with stack unchanged) if there is a problem
 */

static struct INODE_STACK *stack_inode(struct INODE_STACK *topni,
			ntfschar *name, int len, BOOL fix)
{
	struct INODE_STACK *curni;
	u64 inum;

	if (fix)
		inum = ntfs_fix_file_name(topni->ni, name, len);
	else
		inum = ntfs_inode_lookup_by_name(topni->ni, name, len);
	if (inum != (u64)-1) {
		inum = MREF(inum);
		curni = (struct INODE_STACK*)malloc(sizeof(struct INODE_STACK));
		if (curni) {
			curni->ni = ntfs_inode_open(topni->ni->vol, inum);
			topni->next = curni;
			curni->previous = topni;
			curni->next = (struct INODE_STACK*)NULL;
		}
	} else
		curni = (struct INODE_STACK*)NULL;
	return (curni);
}

/*
 *		Destack and close the current inode in the path
 *
 *	Returns the new top of stack
 *		or NULL (with stack unchanged) if there is a problem
 */

static struct INODE_STACK *pop_inode(struct INODE_STACK *topni)
{
	struct INODE_STACK *curni;

	curni = (struct INODE_STACK*)NULL;
	if (topni->previous) {
		if (!ntfs_inode_close(topni->ni)) {
			curni = topni->previous;
			free(topni);
			curni->next = (struct INODE_STACK*)NULL;
		}
	} else {
		/* ".." reached the root of fs */
		errno = ENOENT;
	}
	return (curni);
}

/*
 *		Search for a symbolic link along the target path,
 *	with the target defined as a relative path
 *
 *	Returns the path translated to a Linux path
 *		or NULL if the path is not valid
 */

static char *search_relative(ntfs_volume *vol, ntfschar *path, int count,
		const char *base, BOOL isdir)
{
	struct INODE_STACK *topni;
	struct INODE_STACK *curni;
	char *target;
	ntfschar *unicode;
	int unisz;
	int start;
	int len;

	target = (char*)NULL; /* default return */
	topni = (struct INODE_STACK*)malloc(sizeof(struct INODE_STACK));
	if (topni) {
		topni->ni = ntfs_inode_open(vol, FILE_root);
		topni->previous = (struct INODE_STACK*)NULL;
		topni->next = (struct INODE_STACK*)NULL;
	}
	if (topni && topni->ni) {
		/*
		 * Process the base path
		 */
		unicode = (ntfschar*)NULL;
		unisz = ntfs_mbstoucs(base, &unicode);
		if ((unisz > 0) && unicode) {
			start = 1;
			do {
				len = 0;
				while (((start + len) < unisz)
				    && (unicode[start + len]
					!= const_cpu_to_le16('/')))
						len++;
				curni = (struct INODE_STACK*)NULL;
				if ((start + len) < unisz) {
					curni = stack_inode(topni,
						&unicode[start], len, FALSE);
					if (curni)
						topni = curni;
				} else
					curni = topni;
				start += len + 1;
			} while (curni
			    && topni->ni
			    && (topni->ni->mrec->flags
				& MFT_RECORD_IS_DIRECTORY)
			    && (start < unisz));
			free(unicode);
			if (curni
			    && topni->ni
			    && (topni->ni->mrec->flags
				& MFT_RECORD_IS_DIRECTORY)) {
					start = 0;
				do {
					len = 0;
					while (((start + len) < count)
					    && (path[start + len]
						!= const_cpu_to_le16('\\')))
							len++;
					curni = (struct INODE_STACK*)NULL;
					if ((path[start]
						== const_cpu_to_le16('.'))
					    && ((len == 1)
						|| ((len == 2)
						    && (path[start+1] 
							== const_cpu_to_le16('.'))))) {
					/* leave the .. or . in the path */
						curni = topni;
						if (len == 2) {
							curni = pop_inode(topni);
							if (curni)
								topni = curni;
						}
					} else {
						curni = stack_inode(topni,
						    &path[start], len, TRUE);
						if (curni)
							topni = curni;
					}
					if (topni->ni) {
						start += len;
						if (start < count)
							path[start++]
							    = const_cpu_to_le16('/');
					}
				} while (curni
				    && topni->ni
				    && (topni->ni->mrec->flags
					& MFT_RECORD_IS_DIRECTORY)
				    && (start < count));
				if (curni
				    && topni->ni
				    && (topni->ni->mrec->flags
					 & MFT_RECORD_IS_DIRECTORY
					     ? isdir : !isdir)) {
					if (ntfs_ucstombs(path, count,
					    &target, 0) < 0) {
						if (target) {
							free(target);
							target = (char*)NULL;
						}
					}
				}
			}
		}
		do {
			if (topni->ni)
				ntfs_inode_close(topni->ni);
			curni = topni;
			topni = topni->previous;
			free(curni);
		} while (topni);
	}
	return (target);
}

/*
 *		Check whether a drive letter has been defined in .NTFS-3G
 *
 *	Returns 1 if found,
 *		0 if not found,
 *		-1 if there was an error (described by errno)
 */

static int ntfs_drive_letter(ntfs_volume *vol, ntfschar letter)
{
	char defines[NTFS_MAX_NAME_LEN + 5];
	char *drive;
	int ret;
	int sz;
	int olderrno;
	ntfs_inode *ni;

	ret = -1;
	drive = (char*)NULL;
	sz = ntfs_ucstombs(&letter, 1, &drive, 0);
	if (sz > 0) {
		strcpy(defines,mappingdir);
		if ((*drive >= 'a') && (*drive <= 'z'))
			*drive += 'A' - 'a';
		strcat(defines,drive);
		strcat(defines,":");
		olderrno = errno;
		ni = ntfs_pathname_to_inode(vol, NULL, defines);
		if (ni && !ntfs_inode_close(ni))
			ret = 1;
		else
			if (errno == ENOENT) {
				ret = 0;
					/* avoid errno pollution */
				errno = olderrno;
			}
	}
	if (drive)
		free(drive);
	return (ret);
}

/*
 *		Check and translate the target of a junction point or
 *	a full absolute symbolic link.
 *
 *	A full target definition begins with "\??\" or "\\?\"
 *
 *	The fully defined target is redefined as a relative link,
 *		- either to the target if found on the same device.
 *		- or into the /.NTFS-3G directory for the user to define
 *	In the first situation, the target is translated to case-sensitive path.
 *
 *	returns the target converted to a relative symlink
 *		or NULL if there were some problem, as described by errno
 */


static char *ntfs_get_fulllink(ntfs_volume *vol, ntfschar *junction,
			int count, const char *path, BOOL isdir)
{
	char *target;
	char *fulltarget;
	int i;
	int sz;
	int level;
	const char *p;
	char *q;
	enum { DIR_JUNCTION, VOL_JUNCTION, NO_JUNCTION } kind;

	target = (char*)NULL;
	fulltarget = (char*)NULL;
			/*
			 * For a valid directory junction we want \??\x:\
			 * where \ is an individual char and x a non-null char
			 */
	if ((count >= 7)
	    && !memcmp(junction,dir_junction_head,8)
	    && junction[4]
	    && (junction[5] == const_cpu_to_le16(':'))
	    && (junction[6] == const_cpu_to_le16('\\')))
		kind = DIR_JUNCTION;
	else
			/*
			 * For a valid volume junction we want \\?\Volume{
			 * and a final \ (where \ is an individual char)
			 */
		if ((count >= 12)
		    && !memcmp(junction,vol_junction_head,22)
		    && (junction[count-1] == const_cpu_to_le16('\\')))
			kind = VOL_JUNCTION;
		else
			kind = NO_JUNCTION;
			/*
			 * Directory junction with an explicit path and
			 * no specific definition for the drive letter :
			 * try to interpret as a target on the same volume
			 */
	if ((kind == DIR_JUNCTION)
	    && (count >= 7)
	    && junction[7]
	    && !ntfs_drive_letter(vol, junction[4])) {
		target = search_absolute(vol,&junction[7],count - 7, isdir);
		if (target) {
			level = 0;
			for (p=path; *p; p++)
				if (*p == '/')
					level++;
			fulltarget = (char*)ntfs_malloc(3*level
					+ strlen(target) + 1);
			if (fulltarget) {
				fulltarget[0] = 0;
				if (level > 1) {
					for (i=1; i<level; i++)
						strcat(fulltarget,"../");
				} else
					strcpy(fulltarget,"./");
				strcat(fulltarget,target);
			}
			free(target);
		}
	}
			/*
			 * Volume junctions or directory junctions with
			 * target not found on current volume :
			 * link to /.NTFS-3G/target which the user can
			 * define as a symbolic link to the real target
			 */
	if (((kind == DIR_JUNCTION) && !fulltarget)
	    || (kind == VOL_JUNCTION)) {
		sz = ntfs_ucstombs(&junction[4],
			(kind == VOL_JUNCTION ? count - 5 : count - 4),
			&target, 0);
		if ((sz > 0) && target) {
				/* reverse slashes */
			for (q=target; *q; q++)
				if (*q == '\\')
					*q = '/';
				/* force uppercase drive letter */
			if ((target[1] == ':')
			    && (target[0] >= 'a')
			    && (target[0] <= 'z'))
				target[0] += 'A' - 'a';
			level = 0;
			for (p=path; *p; p++)
				if (*p == '/')
					level++;
			fulltarget = (char*)ntfs_malloc(3*level
					+ sizeof(mappingdir) + count - 4);
			if (fulltarget) {
				fulltarget[0] = 0;
				if (level > 1) {
					for (i=1; i<level; i++)
						strcat(fulltarget,"../");
				} else
					strcpy(fulltarget,"./");
				strcat(fulltarget,mappingdir);
				strcat(fulltarget,target);
			}
		}
		if (target)
			free(target);
	}
	return (fulltarget);
}

/*
 *		Check and translate the target of an absolute symbolic link.
 *
 *	An absolute target definition begins with "\" or "x:\"
 *
 *	The absolute target is redefined as a relative link,
 *		- either to the target if found on the same device.
 *		- or into the /.NTFS-3G directory for the user to define
 *	In the first situation, the target is translated to case-sensitive path.
 *
 *	returns the target converted to a relative symlink
 *		or NULL if there were some problem, as described by errno
 */

static char *ntfs_get_abslink(ntfs_volume *vol, ntfschar *junction,
			int count, const char *path, BOOL isdir)
{
	char *target;
	char *fulltarget;
	int i;
	int sz;
	int level;
	const char *p;
	char *q;
	enum { FULL_PATH, ABS_PATH, REJECTED_PATH } kind;

	target = (char*)NULL;
	fulltarget = (char*)NULL;
			/*
			 * For a full valid path we want x:\
			 * where \ is an individual char and x a non-null char
			 */
	if ((count >= 3)
	    && junction[0]
	    && (junction[1] == const_cpu_to_le16(':'))
	    && (junction[2] == const_cpu_to_le16('\\')))
		kind = FULL_PATH;
	else
			/*
			 * For an absolute path we want an initial \
			 */
		if ((count >= 0)
		    && (junction[0] == const_cpu_to_le16('\\')))
			kind = ABS_PATH;
		else
			kind = REJECTED_PATH;
			/*
			 * Full path, with a drive letter and
			 * no specific definition for the drive letter :
			 * try to interpret as a target on the same volume.
			 * Do the same for an abs path with no drive letter.
			 */
	if (((kind == FULL_PATH)
	    && (count >= 3)
	    && junction[3]
	    && !ntfs_drive_letter(vol, junction[0]))
	    || (kind == ABS_PATH)) {
		if (kind == ABS_PATH)
			target = search_absolute(vol, &junction[1],
				count - 1, isdir);
		else
			target = search_absolute(vol, &junction[3],
				count - 3, isdir);
		if (target) {
			level = 0;
			for (p=path; *p; p++)
				if (*p == '/')
					level++;
			fulltarget = (char*)ntfs_malloc(3*level
					+ strlen(target) + 1);
			if (fulltarget) {
				fulltarget[0] = 0;
				if (level > 1) {
					for (i=1; i<level; i++)
						strcat(fulltarget,"../");
				} else
					strcpy(fulltarget,"./");
				strcat(fulltarget,target);
			}
			free(target);
		}
	}
			/*
			 * full path with target not found on current volume :
			 * link to /.NTFS-3G/target which the user can
			 * define as a symbolic link to the real target
			 */
	if ((kind == FULL_PATH) && !fulltarget) {
		sz = ntfs_ucstombs(&junction[0],
			count,&target, 0);
		if ((sz > 0) && target) {
				/* reverse slashes */
			for (q=target; *q; q++)
				if (*q == '\\')
					*q = '/';
				/* force uppercase drive letter */
			if ((target[1] == ':')
			    && (target[0] >= 'a')
			    && (target[0] <= 'z'))
				target[0] += 'A' - 'a';
			level = 0;
			for (p=path; *p; p++)
				if (*p == '/')
					level++;
			fulltarget = (char*)ntfs_malloc(3*level
					+ sizeof(mappingdir) + count - 4);
			if (fulltarget) {
				fulltarget[0] = 0;
				if (level > 1) {
					for (i=1; i<level; i++)
						strcat(fulltarget,"../");
				} else
					strcpy(fulltarget,"./");
				strcat(fulltarget,mappingdir);
				strcat(fulltarget,target);
			}
		}
		if (target)
			free(target);
	}
	return (fulltarget);
}

/*
 *		Check and translate the target of a relative symbolic link.
 *
 *	A relative target definition does not begin with "\"
 *
 *	The original definition of relative target is kept, it is just
 *	translated to a case-sensitive path.
 *
 *	returns the target converted to a relative symlink
 *		or NULL if there were some problem, as described by errno
 */

static char *ntfs_get_rellink(ntfs_volume *vol, ntfschar *junction,
			int count, const char *path, BOOL isdir)
{
	char *target;

	target = search_relative(vol,junction,count,path,isdir);
	return (target);
}

/*
 *		Get the target for a junction point or symbolic link
 *	Should only be called for files or directories with reparse data
 *
 *	returns the target converted to a relative path, or NULL
 *		if some error occurred, as described by errno
 *		errno is EOPNOTSUPP if the reparse point is not a valid
 *			symbolic link or directory junction
 */

char *ntfs_make_symlink(const char *org_path,
			ntfs_inode *ni,	int *pattr_size)
{
	s64 attr_size = 0;
	char *target;
	unsigned int offs;
	unsigned int lth;
	ntfs_volume *vol;
	REPARSE_POINT *reparse_attr;
	struct MOUNT_POINT_REPARSE_DATA *mount_point_data;
	struct SYMLINK_REPARSE_DATA *symlink_data;
	enum { FULL_TARGET, ABS_TARGET, REL_TARGET } kind;
	ntfschar *p;
	BOOL bad;
	BOOL isdir;

	target = (char*)NULL;
	bad = TRUE;
	isdir = (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)
			 != const_cpu_to_le16(0);
	vol = ni->vol;
	reparse_attr = (REPARSE_POINT*)ntfs_attr_readall(ni,
			AT_REPARSE_POINT,(ntfschar*)NULL, 0, &attr_size);
	if (reparse_attr && attr_size) {
		switch (reparse_attr->reparse_tag) {
		case IO_REPARSE_TAG_MOUNT_POINT :
			mount_point_data = (struct MOUNT_POINT_REPARSE_DATA*)
						reparse_attr->reparse_data;
			offs = le16_to_cpu(mount_point_data->subst_name_offset);
			lth = le16_to_cpu(mount_point_data->subst_name_length);
				/* consistency checks */
			if (isdir
			    && ((le16_to_cpu(reparse_attr->reparse_data_length)
				 + 8) == attr_size)
			    && ((int)((sizeof(REPARSE_POINT)
				 + sizeof(struct MOUNT_POINT_REPARSE_DATA)
				 + offs + lth)) <= attr_size)) {
				target = ntfs_get_fulllink(vol,
					(ntfschar*)&mount_point_data->path_buffer[offs],
					lth/2, org_path, isdir);
				if (target)
					bad = FALSE;
			}
			break;
		case IO_REPARSE_TAG_SYMLINK :
			symlink_data = (struct SYMLINK_REPARSE_DATA*)
						reparse_attr->reparse_data;
			offs = le16_to_cpu(symlink_data->subst_name_offset);
			lth = le16_to_cpu(symlink_data->subst_name_length);
			p = (ntfschar*)&symlink_data->path_buffer[offs];
				/*
				 * Predetermine the kind of target,
				 * the called function has to make a full check
				 */
			if (*p++ == const_cpu_to_le16('\\')) {
				if ((*p == const_cpu_to_le16('?'))
				    || (*p == const_cpu_to_le16('\\')))
					kind = FULL_TARGET;
				else
					kind = ABS_TARGET;
			} else
				if (*p == const_cpu_to_le16(':'))
					kind = ABS_TARGET;
				else
					kind = REL_TARGET;
			p--;
				/* consistency checks */
			if (((le16_to_cpu(reparse_attr->reparse_data_length)
				 + 8) == attr_size)
			    && ((int)((sizeof(REPARSE_POINT)
				 + sizeof(struct SYMLINK_REPARSE_DATA)
				 + offs + lth)) <= attr_size)) {
				switch (kind) {
				case FULL_TARGET :
					if (!(symlink_data->flags
					   & const_cpu_to_le32(1))) {
						target = ntfs_get_fulllink(vol,
							p, lth/2,
							org_path, isdir);
						if (target)
							bad = FALSE;
					}
					break;
				case ABS_TARGET :
					if (symlink_data->flags
					   & const_cpu_to_le32(1)) {
						target = ntfs_get_abslink(vol,
							p, lth/2,
							org_path, isdir);
						if (target)
							bad = FALSE;
					}
					break;
				case REL_TARGET :
					if (symlink_data->flags
					   & const_cpu_to_le32(1)) {
						target = ntfs_get_rellink(vol,
							p, lth/2,
							org_path, isdir);
						if (target)
							bad = FALSE;
					}
					break;
				}
			}
			break;
		}
		free(reparse_attr);
	}
	*pattr_size = attr_size;
	if (bad)
		errno = EOPNOTSUPP;
	return (target);
}

/*
 *		Check whether a reparse point looks like a junction point
 *	or a symbolic link.
 *	Should only be called for files or directories with reparse data
 *
 *	The validity of the target is not checked.
 */

BOOL ntfs_possible_symlink(ntfs_inode *ni)
{
	s64 attr_size = 0;
	REPARSE_POINT *reparse_attr;
	BOOL possible;

	possible = FALSE;
	reparse_attr = (REPARSE_POINT*)ntfs_attr_readall(ni,
			AT_REPARSE_POINT,(ntfschar*)NULL, 0, &attr_size);
	if (reparse_attr && attr_size) {
		switch (reparse_attr->reparse_tag) {
		case IO_REPARSE_TAG_MOUNT_POINT :
		case IO_REPARSE_TAG_SYMLINK :
			possible = TRUE;
		default : ;
		}
		free(reparse_attr);
	}
	return (possible);
}

/*
 *		Get the ntfs reparse data into an extended attribute
 *
 *	Returns the reparse data size
 *		and the buffer is updated if it is long enough
 */

int ntfs_get_ntfs_reparse_data(const char *path  __attribute__((unused)),
			char *value, size_t size, ntfs_inode *ni)
{
	REPARSE_POINT *reparse_attr;
	s64 attr_size;

	attr_size = 0;	/* default to no data and no error */
	if (ni) {
		if (ni->flags & FILE_ATTR_REPARSE_POINT) {
			reparse_attr = (REPARSE_POINT*)ntfs_attr_readall(ni,
				AT_REPARSE_POINT,(ntfschar*)NULL, 0, &attr_size);
			if (reparse_attr) {
				if (attr_size <= (s64)size) {
					if (value)
						memcpy(value,reparse_attr,
							attr_size);
					else
						errno = EINVAL;
				}
				free(reparse_attr);
			}
		} else
			errno = ENODATA;
	}
	return (attr_size ? (int)attr_size : -errno);
}

/*
 *		Set the reparse data from an extended attribute
 *
 *	Warning : the new data is not checked
 *
 *	Returns 0, or -1 if there is a problem
 */

int ntfs_set_ntfs_reparse_data(const char *path  __attribute__((unused)),
			const char *value, size_t size, int flags,
			ntfs_inode *ni)
{
	int res;
	int written;
	ntfs_attr *na;

	res = 0;
	if (ni && (value || !size)) {
		if (!ntfs_attr_exist(ni,AT_REPARSE_POINT,AT_UNNAMED,0)) {
			if (!(flags & XATTR_REPLACE)) {
			/*
			 * no reparse data attribute : add one,
			 * apparently, this does not feed the new value in
			 * Note : NTFS version must be >= 3
			 */
				if (ni->vol->major_ver >= 3) {
					res = ntfs_attr_add(ni,AT_REPARSE_POINT,
						AT_UNNAMED,0,(u8*)NULL,
						(s64)size);
					if (!res)
						ni->flags
						    |= FILE_ATTR_REPARSE_POINT;
					NInoSetDirty(ni);
				} else {
					errno = EOPNOTSUPP;
					res = -1;
				}
			} else {
				errno = ENODATA;
				res = -1;
			}
		} else {
			if (flags & XATTR_CREATE) {
				errno = EEXIST;
				res = -1;
			}
		}
		if (!res) {
			/*
			 * open and update the existing reparse data
			 */
			na = ntfs_attr_open(ni, AT_REPARSE_POINT,
				AT_UNNAMED,0);
			if (na) {
				/* resize attribute */
				res = ntfs_attr_truncate(na, (s64)size);
				/* overwrite value if any */
				if (!res && value) {
					written = (int)ntfs_attr_pwrite(na,
						 (s64)0, (s64)size, value);
					if (written != (s64)size) {
						ntfs_log_error("Failed to update "
							"reparse data\n");
						errno = EIO;
						res = -1;
					}
				}
				ntfs_attr_close(na);
				NInoSetDirty(ni);
			} else
				res = -1;
		}
	} else {
		errno = EINVAL;
		res = -1;
	}
	return (res ? -1 : 0);
}

/*
 *		Remove the reparse data
 *
 *	Returns 0, or -1 if there is a problem
 */

int ntfs_remove_ntfs_reparse_data(const char *path  __attribute__((unused)),
				ntfs_inode *ni)
{
	int res;
	int olderrno;
	ntfs_attr *na;

	res = 0;
	if (ni) {
		/*
		 * open and delete the reparse data
		 */
		na = ntfs_attr_open(ni, AT_REPARSE_POINT,
			AT_UNNAMED,0);
		if (na) {
			/* remove attribute */
			res = ntfs_attr_rm(na);
			if (!res)
				ni->flags &= ~FILE_ATTR_REPARSE_POINT;
			olderrno = errno;
			ntfs_attr_close(na);
					/* avoid errno pollution */
			if (errno == ENOENT)
				errno = olderrno;
		} else {
			errno = ENODATA;
			res = -1;
		}
		NInoSetDirty(ni);
	} else {
		errno = EINVAL;
		res = -1;
	}
	return (res ? -1 : 0);
}

