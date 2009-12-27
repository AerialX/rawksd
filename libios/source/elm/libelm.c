#if 0
#include <sys/iosupport.h>
#include <unistd.h>
#include <string.h>

#include <elm/diskio.h>
#include <elm/ff.h>
#include <elm.h>

static ELM_FATFS _elmsd;
static ELM_FATFS _elmusb;
static ELM_FATFS _elmgca;
static ELM_FATFS _elmgcb;

static const devoptab_t dotab_elmsd = {
	"sd",
	sizeof(FIL),
	_ELMSD_open_r,
	_ELMSD_close_r,
	_ELMSD_write_r,
	_ELMSD_read_r,
	_ELMSD_seek_r,
	_ELMSD_fstat_r,
	_ELMSD_stat_r,
	_ELMSD_link_r,
	_ELMSD_unlink_r,
	_ELMSD_chdir_r,
	_ELMSD_rename_r,
	_ELMSD_mkdir_r,
	sizeof(DIR),
	_ELMSD_diropen_r,
	_ELMSD_dirreset_r,
	_ELMSD_dirnext_r,
	_ELMSD_dirclose_r,
	_ELMSD_statvfs_r,
	_ELMSD_ftruncate_r,
	_ELMSD_fsync_r,
	NULL	/* Device data */
};

static const devoptab_t dotab_elmusb = {
	"usb",
	sizeof(FIL),
	_ELMUSB_open_r,
	_ELMUSB_close_r,
	_ELMUSB_write_r,
	_ELMUSB_read_r,
	_ELMUSB_seek_r,
	_ELMUSB_fstat_r,
	_ELMUSB_stat_r,
	_ELMUSB_link_r,
	_ELMUSB_unlink_r,
	_ELMUSB_chdir_r,
	_ELMUSB_rename_r,
	_ELMUSB_mkdir_r,
	sizeof(DIR),
	_ELMUSB_diropen_r,
	_ELMUSB_dirreset_r,
	_ELMUSB_dirnext_r,
	_ELMUSB_dirclose_r,
	_ELMUSB_statvfs_r,
	_ELMUSB_ftruncate_r,
	_ELMUSB_fsync_r,
	NULL	/* Device data */
};

static const devoptab_t dotab_elmgca = {
	"gca",
	sizeof(FIL),
	_ELMGCA_open_r,
	_ELMGCA_close_r,
	_ELMGCA_write_r,
	_ELMGCA_read_r,
	_ELMGCA_seek_r,
	_ELMGCA_fstat_r,
	_ELMGCA_stat_r,
	_ELMGCA_link_r,
	_ELMGCA_unlink_r,
	_ELMGCA_chdir_r,
	_ELMGCA_rename_r,
	_ELMGCA_mkdir_r,
	sizeof(DIR),
	_ELMGCA_diropen_r,
	_ELMGCA_dirreset_r,
	_ELMGCA_dirnext_r,
	_ELMGCA_dirclose_r,
	_ELMGCA_statvfs_r,
	_ELMGCA_ftruncate_r,
	_ELMGCA_fsync_r,
	NULL	/* Device data */
};

static const devoptab_t dotab_elmgcb = {
	"gcb",
	sizeof(FIL),
	_ELMGCB_open_r,
	_ELMGCB_close_r,
	_ELMGCB_write_r,
	_ELMGCB_read_r,
	_ELMGCB_seek_r,
	_ELMGCB_fstat_r,
	_ELMGCB_stat_r,
	_ELMGCB_link_r,
	_ELMGCB_unlink_r,
	_ELMGCB_chdir_r,
	_ELMGCB_rename_r,
	_ELMGCB_mkdir_r,
	sizeof(DIR),
	_ELMGCB_diropen_r,
	_ELMGCB_dirreset_r,
	_ELMGCB_dirnext_r,
	_ELMGCB_dirclose_r,
	_ELMGCB_statvfs_r,
	_ELMGCB_ftruncate_r,
	_ELMGCB_fsync_r,
	NULL	/* Device data */
};


bool ELM_Mount(char *device)
{
	devoptab_t* devops;
	char* nameCopy;
	int i;
	int strsz;
	int dev = ELM_SD;
	strsz = strlen(device);
	for(i = 0; i < strsz; i++) {
		device[i] = tolower(device[i]);
	}
	if(strncmp(device, "sd", strsz) == 0) {
		if(f_mount(dev, &_elmsd) != FR_OK) {
			return false;
		}
	}
	if(strncmp(device, "usb", strsz) == 0) {
		dev = ELM_USB;
		if(f_mount(dev, &_elmusb) != FR_OK) {
			return false;
		}
	}
	if(strncmp(device, "gca", strsz) == 0) {
		dev = ELM_GCA;
		if(f_mount(dev, &_elmgca) != FR_OK) {
			return false;
		}
	}
	if(strncmp(device, "gcb", strsz) == 0) {
		dev = ELM_GCB;
		if(f_mount(dev, &_elmgcb) != FR_OK) {
			return false;
		}
	}
	devops = malloc(sizeof(devoptab_t) + strlen(name) + 1);
	if(!devops) {
		f_mount(dev, NULL);
		return false;
	}
	nameCopy = (char*)(devops+1);
	switch(dev) {
		case ELM_SD:
			memcpy(devops, &dotab_elmsd,  sizeof(dotab_elmsd));
			break;
		case ELM_USB:
			memcpy(devops, &dotab_elmusb, sizeof(dotab_elmusb));
			break;
		case ELM_GCA:
			memcpy(devops, &dotab_elmgca, sizeof(dotab_elmgca));
			break;
		case ELM_GCB:
			memcpy(devops, &dotab_elmgcb, sizeof(dotab_elmgcb));
			break;
	}
	strcpy(nameCopy, name);
	devops->name = nameCopy;
	devops->deviceData = partition;
	AddDevice(devops);
	return true;
}

void ELM_Unmount(char* device)
{
	int i;
	int strsz;
	int dev = ELM_SD;
	strsz = strlen(device);
	for(i = 0; i < strsz; i++) {
		device[i] = tolower(device[i]);
	}
	if(strncmp(device, "usb", strsz) == 0)
		dev = ELM_USB;
	if(strncmp(device, "gca", strsz) == 0)
		dev = ELM_GCA;
	if(strncmp(device, "gcb", strsz) == 0)
		dev = ELM_GCB;
	if(RemoveDevice(device) == -1) {
		return;
	}
	f_mount(dev, NULL);
}

/* SD Card */
int _ELMSD_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	
}

int _ELMSD_close_r(struct _reent *r, int fd)
{
	
}

ssize_t _ELMSD_write_r(struct _reent *r, int fd, const char *ptr, size_t len)
{
	
}

ssize_t _ELMSD_read_r(struct _reent *r, int fd, char *ptr, size_t len)
{
	
}

off_t _ELMSD_seek_r(struct _reent *r, int fd, off_t pos, int dir)
{
	
}

int _ELMSD_fstat_r(struct _reent *r, int fd, struct stat *st)
{
	
}

int _ELMSD_stat_r(struct _reent *r, const char *file, struct stat *st)
{
	
}

int _ELMSD_link_r(struct _reent *r, const char *existing, const char  *newLink)
{
	
}

int _ELMSD_unlink_r(struct _reent *r, const char *name)
{
	
}

int _ELMSD_chdir_r(struct _reent *r, const char *name)
{
	
}

int _ELMSD_rename_r(struct _reent *r, const char *oldName, const char *newName)
{
	
}

int _ELMSD_mkdir_r(struct _reent *r, const char *path, int mode)
{
	
}

DIR_ITER* _ELMSD_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	
}

int _ELMSD_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMSD_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	
}

int _ELMSD_dirclose_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMSD_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf)
{
	
}

int _ELMSD_ftruncate_r(struct _reent *r, int fd, off_t len)
{
	
}

int _ELMSD_fsync_r(struct _reent *r,int fd)
{
	
}


/* USB */
int _ELMUSB_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	
}

int _ELMUSB_close_r(struct _reent *r, int fd)
{
	
}

ssize_t _ELMUSB_write_r(struct _reent *r, int fd, const char *ptr, size_t len)
{
	
}

ssize_t _ELMUSB_read_r(struct _reent *r, int fd, char *ptr, size_t len)
{
	
}

off_t _ELMUSB_seek_r(struct _reent *r, int fd, off_t pos, int dir)
{
	
}

int _ELMUSB_fstat_r(struct _reent *r, int fd, struct stat *st)
{
	
}

int _ELMUSB_stat_r(struct _reent *r, const char *file, struct stat *st)
{
	
}

int _ELMUSB_link_r(struct _reent *r, const char *existing, const char  *newLink)
{
	
}

int _ELMUSB_unlink_r(struct _reent *r, const char *name)
{
	
}

int _ELMUSB_chdir_r(struct _reent *r, const char *name)
{
	
}

int _ELMUSB_rename_r(struct _reent *r, const char *oldName, const char *newName)
{
	
}

int _ELMUSB_mkdir_r(struct _reent *r, const char *path, int mode)
{
	
}

DIR_ITER* _ELMUSB_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	
}

int _ELMUSB_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMUSB_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	
}

int _ELMUSB_dirclose_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMUSB_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf)
{
	
}

int _ELMUSB_ftruncate_r(struct _reent *r, int fd, off_t len)
{
	
}

int _ELMUSB_fsync_r(struct _reent *r,int fd)
{
	
}


/* SDGecko Port A */
int _ELMGCA_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	
}

int _ELMGCA_close_r(struct _reent *r, int fd)
{
	
}

ssize_t _ELMGCA_write_r(struct _reent *r, int fd, const char *ptr, size_t len)
{
	
}

ssize_t _ELMGCA_read_r(struct _reent *r, int fd, char *ptr, size_t len)
{
	
}

off_t _ELMGCA_seek_r(struct _reent *r, int fd, off_t pos, int dir)
{
	
}

int _ELMGCA_fstat_r(struct _reent *r, int fd, struct stat *st)
{
	
}

int _ELMGCA_stat_r(struct _reent *r, const char *file, struct stat *st)
{
	
}

int _ELMGCA_link_r(struct _reent *r, const char *existing, const char  *newLink)
{
	
}

int _ELMGCA_unlink_r(struct _reent *r, const char *name)
{
	
}

int _ELMGCA_chdir_r(struct _reent *r, const char *name)
{
	
}

int _ELMGCA_rename_r(struct _reent *r, const char *oldName, const char *newName)
{
	
}

int _ELMGCA_mkdir_r(struct _reent *r, const char *path, int mode)
{
	
}

DIR_ITER* _ELMGCA_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	
}

int _ELMGCA_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMGCA_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	
}

int _ELMGCA_dirclose_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMGCA_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf)
{
	
}

int _ELMGCA_ftruncate_r(struct _reent *r, int fd, off_t len)
{
	
}

int _ELMGCA_fsync_r(struct _reent *r,int fd)
{
	
}


/* SDGecko Port B */
int _ELMGCB_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	
}

int _ELMGCB_close_r(struct _reent *r, int fd)
{
	
}

ssize_t _ELMGCB_write_r(struct _reent *r, int fd, const char *ptr, size_t len)
{
	
}

ssize_t _ELMGCB_read_r(struct _reent *r, int fd, char *ptr, size_t len)
{
	
}

off_t _ELMGCB_seek_r(struct _reent *r, int fd, off_t pos, int dir)
{
	
}

int _ELMGCB_fstat_r(struct _reent *r, int fd, struct stat *st)
{
	
}

int _ELMGCB_stat_r(struct _reent *r, const char *file, struct stat *st)
{
	
}

int _ELMGCB_link_r(struct _reent *r, const char *existing, const char  *newLink)
{
	
}

int _ELMGCB_unlink_r(struct _reent *r, const char *name)
{
	
}

int _ELMGCB_chdir_r(struct _reent *r, const char *name)
{
	
}

int _ELMGCB_rename_r(struct _reent *r, const char *oldName, const char *newName)
{
	
}

int _ELMGCB_mkdir_r(struct _reent *r, const char *path, int mode)
{
	
}

DIR_ITER* _ELMGCB_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	
}

int _ELMGCB_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMGCB_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	
}

int _ELMGCB_dirclose_r(struct _reent *r, DIR_ITER *dirState)
{
	
}

int _ELMGCB_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf)
{
	
}

int _ELMGCB_ftruncate_r(struct _reent *r, int fd, off_t len)
{
	
}

int _ELMGCB_fsync_r(struct _reent *r,int fd)
{
	
}

#endif
