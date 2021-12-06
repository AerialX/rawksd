#include <string.h>
#include <fcntl.h>

#include "fat.h"
#include "wrapper.h"
#include "ipc.h"
#include "mem.h"
#include "gctypes.h"

#include "fatdir.h"
#include "fatfile.h"

/* Variables */
static struct _reent fReent;


s32 __FAT_GetError(void)
{
	/* Return error code */
	return fReent._errno;
}

s32 FAT_OpenDir(const char* path)
{
	DIR_ITER dir;
	DIR_STATE_STRUCT* state = Alloc(sizeof(DIR_STATE_STRUCT));

	dir.dirStruct = state;

	fReent._errno = 0;

	s32 ret = (s32)_FAT_diropen_r(&fReent, &dir, path);

	if (ret == 0) {
		Dealloc(state);
		return -1;
	}

	return (s32)state;
}

s32 FAT_NextDir(s32 state, char* filename, struct stat* st)
{
	DIR_ITER dir;
	dir.dirStruct = (DIR_STATE_STRUCT*)state;
	fReent._errno = 0;
	return _FAT_dirnext_r(&fReent, &dir, filename, st);
}

s32 FAT_CloseDir(s32 state)
{
	DIR_ITER dir;
	dir.dirStruct = (DIR_STATE_STRUCT*)state;
	fReent._errno = 0;
	s32 ret = _FAT_dirclose_r(&fReent, &dir);
	Dealloc((void*)state);

	return ret;
}

s32 __FAT_OpenDir(const char *dirpath, DIR_ITER *dir)
{
	DIR_ITER         *result = NULL;
	DIR_STATE_STRUCT *state  = NULL;

	/* Allocate memory */
	state = Alloc(sizeof(DIR_STATE_STRUCT));
	if (!state)
		return IPC_ENOMEM;

	/* Clear buffer */
	memset(state, 0, sizeof(DIR_STATE_STRUCT));

	/* Prepare dir iterator */
	dir->device    = 0;
	dir->dirStruct = state;

	/* Clear error code */
	fReent._errno = 0;

	/* Open directory */
	result = _FAT_diropen_r(&fReent, dir, dirpath);

	if (!result) {
		/* Free memory */
		Dealloc(state);

		/* Return error */
		return __FAT_GetError();
	}

	return 0;
}

void __FAT_CloseDir(DIR_ITER *dir)
{
	/* Close directory */
	_FAT_dirclose_r(&fReent, dir);

	/* Free memory */
	Dealloc(dir->dirStruct);
}

s32 FAT_Open(const char *path, u32 mode)
{
	FILE_STRUCT *fs = Alloc(sizeof(FILE_STRUCT));
	if (!fs)
		return IPC_ENOMEM;

	int ret = FAT_Open_Prealloc(path, mode, fs);

	if (ret < 0)
		Dealloc(fs);

	return ret;
}

s32 FAT_Open_Prealloc(const char *path, u32 mode, FILE_STRUCT* fs)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Open file */
	//ret = _FAT_open_r(&fReent, fs, path, 2, 0);
	ret = _FAT_open_r(&fReent, fs, path, mode, 0);
	if (ret < 0) {
		return ret;
		return __FAT_GetError();
	}

	return ret;
}

s32 FAT_Close(s32 fd)
{
	FILE_STRUCT *fs = (FILE_STRUCT *)fd;

	/* Close file */
	s32 ret = _FAT_close_r(&fReent, fs);
	Dealloc(fs);
	return ret;
}

s32 FAT_Close_Prealloc(s32 fd)
{
	/* Close file */
	return _FAT_close_r(&fReent, (void*)fd);
}

s32 FAT_Read(s32 fd, void *buffer, u32 len)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Read file */
	ret = _FAT_read_r(&fReent, (void*)fd, buffer, len);
	//if (ret < 0)
	//	ret = __FAT_GetError();

	return ret;
}

s32 FAT_Write(s32 fd, void *buffer, u32 len)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Write file */
	ret = _FAT_write_r(&fReent, (void*)fd, buffer, len);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_Flush(s32 fd)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Sync file */
	ret = _FAT_fsync_r(&fReent, (void*)fd);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_Seek(s32 fd, u32 where, u32 whence)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Seek file */
	ret = _FAT_seek_r(&fReent, (void*)fd, where, whence);
	//if (ret < 0)
	//	ret = __FAT_GetError();

	return ret;
}

s32 FAT_Tell(s32 fd)
{
	//return (s32)FAT_Seek(fd, 0, SEEK_CUR);
	return (s32)((FILE_STRUCT*)fd)->currentPosition;
}

s32 FAT_CreateDir(const char *dirpath)
{
	s32 ret;

	if (dirpath==NULL)
		return IPC_EINVAL;

	if (!dirpath[0])
		return IPC_OK;

	/* Clear error code */
	fReent._errno = 0;

	/* Create directory */
	ret = _FAT_mkdir_r(&fReent, dirpath, 0);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_CreateFile(const char *filepath)
{
	FILE_STRUCT fs;
	s32         ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Create file */
	ret = _FAT_open_r(&fReent, &fs, filepath, O_CREAT | O_RDWR, 0);
	if (ret < 0)
		return __FAT_GetError();

	/* Close file */
	_FAT_close_r(&fReent, (void*)ret);

	return 0;
}

s32 FAT_ReadDir(const char *dirpath, char *outbuf, u32 *outlen, u32 maxlen)
{
	DIR_ITER dir;

	u32 cnt = 0, pos = 0;
	s32 ret;

	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	if (ret < 0)
		return ret;

	/* Read entries */
	while (!maxlen || (maxlen > cnt)) {
		char *filename = outbuf + pos;

		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, filename, NULL))
			break;

		/* Non valid entry */
		if (!strcmp(filename, ".") || !strcmp(filename, ".."))
			continue;

		/* Increase counter */
		cnt++;

		/* Update position */
		pos += (outbuf) ? strlen(filename) + 1 : 0;
	}

	/* Output values */
	*outlen = cnt;

	/* Close directory */
	__FAT_CloseDir(&dir);

	return 0;
}

s32 FAT_Delete(const char *path)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Delete file/directory */
	ret = _FAT_unlink_r(&fReent, path);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_DeleteDir(const char *dirpath)
{
	DIR_ITER dir;

	s32 ret;

	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	if (ret < 0)
		return ret;

	/* Read entries */
	for (;;) {
		char   filename[MAX_FILENAME_LENGTH], newpath[MAX_FILENAME_LENGTH];
		struct stat filestat;

		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (!strcmp(filename, ".") || !strcmp(filename, ".."))
			continue;

		/* Generate entry path */
		strcpy(newpath, dirpath);
		strcat(newpath, "/");
		strcat(newpath, filename);

		/* Delete directory contents */
		if (filestat.st_mode & S_IFDIR)
			FAT_DeleteDir(newpath);

		/* Delete object */
		ret = FAT_Delete(newpath);

		/* Error */
		if (ret < 0)
			break;
	}

	/* Close directory */
	__FAT_CloseDir(&dir);

	return 0;
}

s32 FAT_Rename(const char *oldname, const char *newname)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Rename file/directory */
	ret = _FAT_rename_r(&fReent, oldname, newname);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_Stat(const char *path, struct stat *stats)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	struct stat st;
	if (stats == NULL)
		stats = &st;

	/* Get stats */
	ret = _FAT_stat_r(&fReent, path, stats);
	//if (ret < 0)
	//	ret = __FAT_GetError();

	return ret;
}

s32 FAT_GetVfsStats(const char *path, struct statvfs *stats)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Get filesystem stats */
	ret = _FAT_statvfs_r(&fReent, path, stats);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_GetFileStats(s32 fd, fstats *stats)
{
	FILE_STRUCT *fs = (FILE_STRUCT *)fd;

	if (fs && fs->inUse) {
		/* Fill file stats */
		stats->file_length = fs->filesize;
		stats->file_pos    = fs->currentPosition;

		return 0;
	}

	return IPC_EINVAL;
}

#if 0
s32 FAT_GetUsage(const char *dirpath, u64 *size, u32 *files)
{
	DIR_ITER dir;

	u64 totalSz  = 0;
	u32 totalCnt = 0;
	s32 ret;

	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	if (ret < 0)
		return ret;

	/* Read entries */
	for (;;) {
		char   filename[MAX_FILENAME_LENGTH];
		struct stat filestat;

		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (!strcmp(filename, ".") || !strcmp(filename, ".."))
			continue;

		/* Directory or file */
		if (filestat.st_mode & S_IFDIR) {
			char newpath[MAX_FILENAME_LENGTH];

			u64  dirsize;
			u32  dirfiles;

			/* Generate directory path */
			strcpy(newpath, dirpath);
			strcat(newpath, "/");
			strcat(newpath, filename);

			/* Get directory usage */
			ret = FAT_GetUsage(newpath, &dirsize, &dirfiles);
			if (ret >= 0) {
				/* Update variables */
				totalSz  += dirsize;
				totalCnt += dirfiles;
			}
		} else
			totalSz += filestat.st_size;

		/* Increment counter */
		totalCnt++;
	}

	/* Output values */
	*size  = totalSz;
	*files = totalCnt;

	/* Close directory */
	__FAT_CloseDir(&dir);

	return 0;
}
#endif
