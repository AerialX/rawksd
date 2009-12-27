/*
 * fileio.c --- Simple file I/O routines
 *
 * Copyright (C) 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include "fuse-ext2.h"

#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>

#define EXT2_FILE_SHARED_INODE 0x8000

struct ext2_file {
	errcode_t		magic;
	ext2_filsys 		fs;
	ext2_ino_t		ino;
	struct ext2_inode	*inode;
	int 			flags;
	__u64			pos;
	blk_t			blockno;
	blk_t			physblock;
	char 			*buf;
};

#define BMAP_BUFFER (file->buf + fs->blocksize)

errcode_t ext2fs_file_open2(ext2_filsys fs, ext2_ino_t ino,
			    struct ext2_inode *inode,
			    int flags, ext2_file_t *ret)
{
	ext2_file_t 	file;
	errcode_t	retval;

	/*
	 * Don't let caller create or open a file for writing if the
	 * filesystem is read-only.
	 */
	if ((flags & (EXT2_FILE_WRITE | EXT2_FILE_CREATE)) &&
	    !(fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	retval = ext2fs_get_mem(sizeof(struct ext2_file), &file);
	if (retval)
		return retval;

	memset(file, 0, sizeof(struct ext2_file));
	file->magic = EXT2_ET_MAGIC_EXT2_FILE;
	file->fs = fs;
	file->ino = ino;
	file->flags = flags & (EXT2_FILE_MASK | EXT2_FILE_SHARED_INODE);

	if (flags & EXT2_FILE_SHARED_INODE) 
		file->inode = inode;
	else {
		retval = ext2fs_get_mem(sizeof(struct ext2_inode), &file->inode);
		if (retval)
			goto fail_inode_alloc;
		if (inode) {
			memcpy(file->inode, inode, sizeof(struct ext2_inode));
		} else {
			retval = ext2fs_read_inode(fs, ino, file->inode);
			if (retval)
				goto fail;
		}
	}

	retval = ext2fs_get_array(3, fs->blocksize, &file->buf);
	if (retval)
		goto fail;

	*ret = file;
	return 0;

fail:
	if (!(file->flags & EXT2_FILE_SHARED_INODE))
		ext2fs_free_mem(&file->inode);
fail_inode_alloc:
	if (file->buf)
		ext2fs_free_mem(&file->buf);
	ext2fs_free_mem(&file);
	return retval;
}

errcode_t ext2fs_file_open(ext2_filsys fs, ext2_ino_t ino,
			   int flags, ext2_file_t *ret)
{
	return ext2fs_file_open2(fs, ino, NULL, flags & ~(EXT2_FILE_SHARED_INODE), ret);
}

/*
 * This function returns the filesystem handle of a file from the structure
 */
ext2_filsys ext2fs_file_get_fs(ext2_file_t file)
{
	if (file->magic != EXT2_ET_MAGIC_EXT2_FILE)
		return 0;
	return file->fs;
}

/*
 * This function flushes the dirty block buffer out to disk if
 * necessary.
 */
errcode_t ext2fs_file_flush(ext2_file_t file)
{
	errcode_t	retval;
	ext2_filsys fs;

	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);
	fs = file->fs;

	if (!(file->flags & EXT2_FILE_BUF_VALID) ||
	    !(file->flags & EXT2_FILE_BUF_DIRTY))
		return 0;

	/*
	 * OK, the physical block hasn't been allocated yet.
	 * Allocate it.
	 */
	if (!file->physblock) {
		retval = ext2fs_bmap(fs, file->ino, file->inode,
				     BMAP_BUFFER, file->ino ? BMAP_ALLOC : 0,
				     file->blockno, &file->physblock);
		if (retval)
			return retval;
	}

	retval = io_channel_write_blk(fs->io, file->physblock,
				      1, file->buf);
	if (retval)
		return retval;

	file->flags &= ~EXT2_FILE_BUF_DIRTY;

	return retval;
}

/*
 * This function synchronizes the file's block buffer and the current
 * file position, possibly invalidating block buffer if necessary
 */
static errcode_t sync_buffer_position(ext2_file_t file)
{
	blk_t	b;
	errcode_t	retval;

	b = file->pos / file->fs->blocksize;
	if (b != file->blockno) {
		retval = ext2fs_file_flush(file);
		if (retval)
			return retval;
		file->flags &= ~EXT2_FILE_BUF_VALID;
	}
	file->blockno = b;
	return 0;
}

/*
 * This function loads the file's block buffer with valid data from
 * the disk as necessary.
 *
 * If dontfill is true, then skip initializing the buffer since we're
 * going to be replacing its entire contents anyway.  If set, then the
 * function basically only sets file->physblock and EXT2_FILE_BUF_VALID
 */
#define DONTFILL 1
static errcode_t load_buffer(ext2_file_t file, int dontfill)
{
	ext2_filsys	fs = file->fs;
	errcode_t	retval;

	if (!(file->flags & EXT2_FILE_BUF_VALID)) {
		retval = ext2fs_bmap(fs, file->ino, file->inode,
				     BMAP_BUFFER, 0, file->blockno,
				     &file->physblock);
		if (retval)
			return retval;
		if (!dontfill) {
			if (file->physblock) {
				retval = io_channel_read_blk(fs->io,
							     file->physblock,
							     1, file->buf);
				if (retval)
					return retval;
			} else
				memset(file->buf, 0, fs->blocksize);
		}
		file->flags |= EXT2_FILE_BUF_VALID;
	}
	return 0;
}


errcode_t ext2fs_file_close2 (ext2_file_t file, void (*close_callback) (struct ext2_inode *inode, int flags))
{
	errcode_t retval;

	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);

	retval = ext2fs_file_flush(file);

	if (file->buf) {
		ext2fs_free_mem(&file->buf);
	}
	if (!(file->flags & EXT2_FILE_SHARED_INODE)) {
		ext2fs_free_mem(&file->inode);
	} else if (close_callback != NULL) {
		close_callback(file->inode, file->flags & EXT2_FILE_MASK);
	}
	ext2fs_free_mem(&file);

	return retval;
}

errcode_t ext2fs_file_close(ext2_file_t file)
{
	return ext2fs_file_close2(file, NULL);
}

errcode_t ext2fs_file_read(ext2_file_t file, void *buf,
			   unsigned int wanted, unsigned int *got)
{
	ext2_filsys	fs;
	errcode_t	retval = 0;
	unsigned int	start, c, count = 0;
	__u64		left;
	char		*ptr = (char *) buf;

	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);
	fs = file->fs;

	while ((file->pos < EXT2_I_SIZE(file->inode)) && (wanted > 0)) {
		retval = sync_buffer_position(file);
		if (retval)
			goto fail;
		retval = load_buffer(file, 0);
		if (retval)
			goto fail;

		start = file->pos % fs->blocksize;
		c = fs->blocksize - start;
		if (c > wanted)
			c = wanted;
		left = EXT2_I_SIZE(file->inode) - file->pos ;
		if (c > left)
			c = left;

		memcpy(ptr, file->buf+start, c);
		file->pos += c;
		ptr += c;
		count += c;
		wanted -= c;
	}

fail:
	if (got)
		*got = count;
	return retval;
}


errcode_t ext2fs_file_write(ext2_file_t file, const void *buf,
			    unsigned int nbytes, unsigned int *written)
{
	ext2_filsys	fs;
	errcode_t	retval = 0;
	unsigned int	start, c, count = 0;
	const char	*ptr = (const char *) buf;

	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);
	fs = file->fs;

	if (!(file->flags & EXT2_FILE_WRITE))
		return EXT2_ET_FILE_RO;

	while (nbytes > 0) {
		retval = sync_buffer_position(file);
		if (retval)
			goto fail;

		start = file->pos % fs->blocksize;
		c = fs->blocksize - start;
		if (c > nbytes)
			c = nbytes;

		/*
		 * We only need to do a read-modify-update cycle if
		 * we're doing a partial write.
		 */
		retval = load_buffer(file, (c == fs->blocksize));
		if (retval)
			goto fail;

		file->flags |= EXT2_FILE_BUF_DIRTY;
		memcpy(file->buf+start, ptr, c);
		file->pos += c;
		ptr += c;
		count += c;
		nbytes -= c;
	}

fail:
	if (written)
		*written = count;
	return retval;
}

errcode_t ext2fs_file_llseek(ext2_file_t file, __u64 offset,
			    int whence, __u64 *ret_pos)
{
	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);

	if (whence == EXT2_SEEK_SET)
		file->pos = offset;
	else if (whence == EXT2_SEEK_CUR)
		file->pos += offset;
	else if (whence == EXT2_SEEK_END)
		file->pos = EXT2_I_SIZE(file->inode) + offset;
	else
		return EXT2_ET_INVALID_ARGUMENT;

	if (ret_pos)
		*ret_pos = file->pos;

	return 0;
}

errcode_t ext2fs_file_lseek(ext2_file_t file, ext2_off_t offset,
			    int whence, ext2_off_t *ret_pos)
{
	__u64		loffset, ret_loffset;
	errcode_t	retval;

	loffset = offset;
	retval = ext2fs_file_llseek(file, loffset, whence, &ret_loffset);
	if (ret_pos)
		*ret_pos = (ext2_off_t) ret_loffset;
	return retval;
}


/*
 * This function returns the size of the file, according to the inode
 */
errcode_t ext2fs_file_get_lsize(ext2_file_t file, __u64 *ret_size)
{
	if (file->magic != EXT2_ET_MAGIC_EXT2_FILE)
		return EXT2_ET_MAGIC_EXT2_FILE;
	*ret_size = EXT2_I_SIZE(file->inode);
	return 0;
}

/*
 * This function returns the size of the file, according to the inode
 */
ext2_off_t ext2fs_file_get_size(ext2_file_t file)
{
	__u64	size;

	if (ext2fs_file_get_lsize(file, &size))
		return 0;
	if ((size >> 32) != 0)
		return 0;
	return size;
}

struct truncate_st {
	__u32 fileblkcount;
	__u32 newblksize;
	__u8 okind;
	__u8 okdind;
	__u8 oktind;
};

static int truncate_blocks (ext2_filsys fs, blk_t *blocknr, int blockcnt, void *private)
{
	struct truncate_st *tr_data=private;
	blk_t block;
	int keep;

	switch (blockcnt) {
		case BLOCK_COUNT_IND:
			keep = (tr_data->okind > 0);
			tr_data->okind=0;
			break;
		case BLOCK_COUNT_DIND:
			keep = (tr_data->okdind > 0);
			tr_data->okdind=0;
			break;
		case BLOCK_COUNT_TIND:
			keep = (tr_data->oktind > 0);
			break;
		default:
			keep = (blockcnt < tr_data->fileblkcount);
			if (keep) {
				if (blockcnt >= EXT2_NDIR_BLOCKS) {
					int limit = fs->blocksize >> 2;
					tr_data->okind=1;
					if (blockcnt >= EXT2_NDIR_BLOCKS + limit) {
						tr_data->okdind=1;
						if (blockcnt >= EXT2_NDIR_BLOCKS + limit * (limit + 1))
							tr_data->oktind=1;
					}
				}
			}
	}

	if (keep)
		tr_data->newblksize++;
	else {
		block = *blocknr;
		ext2fs_block_alloc_stats(fs, block, -1);
		*blocknr = 0;
		return BLOCK_CHANGED;
	}

	return 0;
}

/*
 * This function sets the size of the file, truncating it if necessary
 *
 */
errcode_t ext2fs_file_set_lsize(ext2_file_t file, __u64 size)
{
	int retval;
	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);

	if (size >= file->inode->i_size) {
		file->inode->i_size = size & 0xffffffff;
		file->inode->i_size_high = (size >> 32) & 0xffffffff;
		if (file->ino) {
			retval = ext2fs_write_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;
		}
	} else {
		unsigned int blocksize=file->fs->blocksize;
		struct truncate_st tr_data={
			.fileblkcount = (size + (blocksize - 1)) / blocksize,
			.newblksize = 0,
			.okind = 0,
			.okdind = 0,
			.oktind = 0};
		char scratchbuf[3 * blocksize];

		ext2fs_file_flush(file);
		file->inode->i_size = size & 0xffffffff;
		file->inode->i_size_high = (size >> 32) & 0xffffffff;
		if (file->ino) {
			retval = ext2fs_write_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;

			ext2fs_block_iterate(file->fs, file->ino, BLOCK_FLAG_DEPTH_TRAVERSE, scratchbuf, truncate_blocks, &tr_data);

			retval = ext2fs_read_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;
			file->inode->i_blocks= tr_data.newblksize * (blocksize / 512);
			retval = ext2fs_write_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;
		}
	}

	return 0;
}

errcode_t ext2fs_file_set_size(ext2_file_t file, ext2_off_t size)
{
	int retval;
	EXT2_CHECK_MAGIC(file, EXT2_ET_MAGIC_EXT2_FILE);

	if (size >= file->inode->i_size) {
		file->inode->i_size = size;
		file->inode->i_size_high = 0;
		if (file->ino) {
			retval = ext2fs_write_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;
		}
	} else {
		unsigned int blocksize=file->fs->blocksize;
		struct truncate_st tr_data={
			.fileblkcount = (size + (blocksize - 1)) / blocksize,
			.newblksize = 0,
			.okind = 0,
			.okdind = 0,
			.oktind = 0};
		char scratchbuf[3 * blocksize];

		ext2fs_file_flush(file);
		file->inode->i_size = size;
		file->inode->i_size_high = 0;
		if (file->ino) {
			retval = ext2fs_write_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;

			ext2fs_block_iterate(file->fs, file->ino, BLOCK_FLAG_DEPTH_TRAVERSE, scratchbuf, truncate_blocks, &tr_data);

			retval = ext2fs_read_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;
			file->inode->i_blocks= tr_data.newblksize * (blocksize / 512);
			retval = ext2fs_write_inode(file->fs, file->ino, file->inode);
			if (retval)
				return retval;
		}
	}

	return 0;
}
