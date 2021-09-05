#include "ABI/afc_internal/afc_filesystem.h"
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
#include <libimobiledevice/house_arrest.h>
#endif
struct afc_error_mapping {
	afc_error_t from;
	int to;
} static afc_error_to_errno_map[] = {
	{AFC_E_SUCCESS               , 0},
	{AFC_E_OP_HEADER_INVALID     , EIO},
	{AFC_E_NO_RESOURCES          , EMFILE},
	{AFC_E_READ_ERROR            , ENOTDIR},
	{AFC_E_WRITE_ERROR           , EIO},
	{AFC_E_UNKNOWN_PACKET_TYPE   , EIO},
	{AFC_E_INVALID_ARG           , EINVAL},
	{AFC_E_OBJECT_NOT_FOUND      , ENOENT},
	{AFC_E_OBJECT_IS_DIR         , EISDIR},
	{AFC_E_DIR_NOT_EMPTY         , ENOTEMPTY},
	{AFC_E_PERM_DENIED           , EPERM},
	{AFC_E_SERVICE_NOT_CONNECTED , ENXIO},
	{AFC_E_OP_TIMEOUT            , ETIMEDOUT},
	{AFC_E_TOO_MUCH_DATA         , EFBIG},
	{AFC_E_END_OF_DATA           , ENODATA},
	{AFC_E_OP_NOT_SUPPORTED      , ENOSYS},
	{AFC_E_OBJECT_EXISTS         , EEXIST},
	{AFC_E_OBJECT_BUSY           , EBUSY},
	{AFC_E_NO_SPACE_LEFT         , ENOSPC},
	{AFC_E_OP_WOULD_BLOCK        , EWOULDBLOCK},
	{AFC_E_IO_ERROR              , EIO},
	{AFC_E_OP_INTERRUPTED        , EINTR},
	{AFC_E_OP_IN_PROGRESS        , EALREADY},
	{AFC_E_INTERNAL_ERROR        , EIO},
	{(afc_error_t)-1,(afc_error_t)-1}
};

namespace afc{
	namespace internal{
		const unsigned long kAccMode = 3;
		void free_dictionary(char **dictionary){
			int i = 0;

			if (!dictionary)
				return;

			for (i = 0; dictionary[i]; i++) {
				free(dictionary[i]);
			}
			free(dictionary);
		}
		int get_afc_error_as_errno(afc_error_t error)
		{
			int i = 0;
			int res = -1;

			while (afc_error_to_errno_map[i++].from != -1) {
				if (afc_error_to_errno_map[i].from == error) {
					res = afc_error_to_errno_map[i++].to;
					break;
				}
			}

			if (res == -1) {
				fprintf(stderr, "Unknown AFC status %d.\n", error);
				res = EIO;
			}

			return res;
		}
		int get_afc_file_mode(afc_file_mode_t *afc_mode, int flags = O_RDONLY){
			switch (flags & kAccMode) {
			case O_RDONLY:
				*afc_mode = AFC_FOPEN_RDONLY;
				break;
			case O_WRONLY:
				if ((flags & O_TRUNC) == O_TRUNC) {
					*afc_mode = AFC_FOPEN_WRONLY;
				} else if ((flags & O_APPEND) == O_APPEND) {
					*afc_mode = AFC_FOPEN_APPEND;
				} else {
					*afc_mode = AFC_FOPEN_RW;
				}
				break;
			case O_RDWR:
				if ((flags & O_TRUNC) == O_TRUNC) {
					*afc_mode = AFC_FOPEN_WR;
				} else if ((flags & O_APPEND) == O_APPEND) {
					*afc_mode = AFC_FOPEN_RDAPPEND;
				} else {
					*afc_mode = AFC_FOPEN_RW;
				}
				break;
			default:
				*afc_mode = (afc_file_mode_t)0;
				return -1;
			}
			return 0;
		}
		int ifuse_getattr(afc_client_t afc,const char *path,struct StatAttr* stbuf){
			int i;
			int res = 0;
			char **info = NULL;
			afc_error_t ret = afc_get_file_info(afc, path, &info);
			memset(stbuf, 0, sizeof(struct stat));
			if (ret != AFC_E_SUCCESS) {
				int e = get_afc_error_as_errno(ret);
				res = -e;
			} else if (!info) {
				res = -1;
			} else {
				// get file attributes from info list
				for (i = 0; info[i]; i += 2) {
					if (!strcmp(info[i], "st_size")) {
						stbuf->st_size = _atoi64(info[i + 1]);
					} else if (!strcmp(info[i], "st_blocks")) {
						stbuf->st_blocks = atoi(info[i+1]);
					} else if (!strcmp(info[i], "st_ifmt")) {
						if (!strcmp(info[i+1], "S_IFREG")) {
							stbuf->st_mode = S_IFREG;
						} else if (!strcmp(info[i+1], "S_IFDIR")) {
							stbuf->st_mode = S_IFDIR;
						} else if (!strcmp(info[i+1], "S_IFLNK")) {
							stbuf->st_mode = S_IFLNK;
						} else if (!strcmp(info[i+1], "S_IFBLK")) {
							stbuf->st_mode = S_IFBLK;
						} else if (!strcmp(info[i+1], "S_IFCHR")) {
							stbuf->st_mode = S_IFCHR;
						} else if (!strcmp(info[i+1], "S_IFIFO")) {
							stbuf->st_mode = S_IFIFO;
						} else if (!strcmp(info[i+1], "S_IFSOCK")) {
							stbuf->st_mode = S_IFSOCK;
						}
					} 
					else if (!strcmp(info[i], "st_nlink")) {
						stbuf->st_nlink = atoi(info[i+1]);
					} 
					else if (!strcmp(info[i], "st_mtime")) {
						stbuf->st_mtime = (time_t)(atof(info[i+1]) / 1000000000);
					}
					else if (!strcmp(info[i], "st_birthtime")) { /* available on iOS 7+ */
						stbuf->st_birthtime = (time_t)(atof(info[i+1]) / 1000000000);
					}
				}
				free_dictionary(info);
				// set permission bits according to the file type
				if (S_ISDIR(stbuf->st_mode)){
					stbuf->st_mode |= 0755;
				} 
				else if (S_ISLNK(stbuf->st_mode)){
					stbuf->st_mode |= 0777;
				} 
				else{
					stbuf->st_mode |= 0644;
				}

				// and set some additional info
				stbuf->st_uid = GetCurrentProcessId();
				stbuf->st_gid = 0;
				StatByte bytes;
				ifuse_statfs(afc,path,&bytes);
				stbuf->st_blksize = bytes.blocksize;
			}

			return res;
		}

		int ifuse_readdir(afc_client_t afc,const char *path,char ***dirs)
		{
			afc_error_t error_code = afc_read_directory(afc, path, dirs);

			if (!dirs||error_code!=AFC_E_SUCCESS){
				return -ENOENT;
			}
			return 0;
		}

		int ifuse_open(afc_client_t afc,const char *path,uint64_t* fi,int flag)
		{
			afc_error_t err;
			afc_file_mode_t mode = (afc_file_mode_t)0;

			err = (afc_error_t)get_afc_file_mode(&mode,flag);
			if (err != AFC_E_SUCCESS || (mode == 0)) {
				*fi = NULL;
				return -EPERM;
			}

			err = afc_file_open(afc, path, mode, fi);
			if (err != AFC_E_SUCCESS) {
				*fi = NULL;
				int res = get_afc_error_as_errno(err);
				return -res;
			}

			return 0;
		}

		int ifuse_create(afc_client_t afc,const char *path,uint64_t *fi,int flag)
		{
			return ifuse_open(afc,path,fi,flag);
		}

		int ifuse_read(afc_client_t afc,char *buf, size_t size, off_t offset,uint64_t fi)
		{
			uint32_t bytes = 0;
			if (size == 0)
				return 0;
			afc_error_t err = afc_file_seek(afc, fi, offset, SEEK_SET);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}

			err = afc_file_read(afc, fi, buf, size, &bytes);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}

			return bytes;
		}

		int ifuse_write(afc_client_t afc,const char *path, const char *buf, size_t size, off_t offset,uint64_t fi)
		{
			uint32_t bytes = 0;
			if (size == 0)
				return 0;

			afc_error_t err = afc_file_seek(afc, fi, offset, SEEK_SET);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}

			err = afc_file_write(afc, fi, buf, size, &bytes);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}

			return bytes;
		}

		int ifuse_utimens(afc_client_t afc,const char *path, const timeval tv[2])
		{
			uint64_t mtime = (uint64_t)tv[1].tv_sec * (uint64_t)1000000000 + (uint64_t)tv[1].tv_sec;

			afc_error_t err = afc_set_file_time(afc, path, mtime);
			if (err == AFC_E_UNKNOWN_PACKET_TYPE) {
				/* ignore error for pre-3.1 devices as they do not support setting file modification times */
				return 0;
			}
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}

			return 0;
		}
		int ifuse_release(afc_client_t afc,uint64_t fi)
		{
			afc_file_close(afc, fi);

			return 0;
		}
		int ifuse_statfs(afc_client_t afc,const char *path,StatByte* bytes){
			char **info_raw = NULL;
			uint64_t totalspace = 0, freespace = 0;
			uint64_t i = 0, blocksize = 0;

			afc_error_t err = afc_get_device_info(afc, &info_raw);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}
			if (!info_raw)
				return -ENOENT;

			for(int i = 0; info_raw[i]; i++) {
				if (!strcmp(info_raw[i], "FSTotalBytes")) {
					totalspace = strtoul(info_raw[i + 1], (char **) NULL, 10);
				} else if (!strcmp(info_raw[i], "FSFreeBytes")) {
					freespace = strtoul(info_raw[i + 1], (char **) NULL, 10);
				} else if (!strcmp(info_raw[i], "FSBlockSize")) {
					blocksize = atoi(info_raw[i + 1]);
				}
			}
			free_dictionary(info_raw);
			if(bytes){
				bytes->totalspace = totalspace;
				bytes->freespace = freespace;
				bytes->blocksize = blocksize;
			}
			return 0;
		}

		int ifuse_truncate(afc_client_t afc,const char *path, off_t size){
			afc_error_t err = afc_truncate(afc, path, size);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}
			return 0;
		}

		int ifuse_ftruncate(afc_client_t afc,const char *path, off_t size,uint64_t fi){
			afc_error_t err = afc_file_truncate(afc, fi, size);
			if (err != AFC_E_SUCCESS) {
				int res = get_afc_error_as_errno(err);
				return -res;
			}
			return 0;
		}

		int ifuse_readlink(afc_client_t afc,const char *path, char *linktarget, size_t buflen){
			int i, ret;
			char **info = NULL;
			if (!path || !linktarget || (buflen == 0)) {
				return -EINVAL;
			}
			linktarget[0] = '\0'; // in case the link target cannot be determined
			afc_error_t err = afc_get_file_info(afc, path, &info);
			if ((err == AFC_E_SUCCESS) && info) {
				ret = -1;
				for (i = 0; info[i]; i+=2) {
					if (!strcmp(info[i], "LinkTarget")) {
						strncpy(linktarget, info[i+1], buflen-1);
						linktarget[buflen-1] = '\0';
						ret = 0;
					}
				}
				free_dictionary(info);
			} else {
				ret = get_afc_error_as_errno(err);
				return -ret;
			}

			return ret;
		}

		int ifuse_symlink(afc_client_t afc,const char *target, const char *linkname){
			afc_error_t err = afc_make_link(afc, AFC_SYMLINK, target, linkname);
			if (err == AFC_E_SUCCESS){
				return 0;
			}
			return -get_afc_error_as_errno(err);
		}

		int ifuse_link(afc_client_t afc,const char *target, const char *linkname){
			afc_error_t err = afc_make_link(afc, AFC_HARDLINK, target, linkname);
			if (err == AFC_E_SUCCESS){
				return 0;
			}
			return -get_afc_error_as_errno(err);
		}

		int ifuse_unlink(afc_client_t afc,const char *path){
			afc_error_t err = afc_remove_path(afc, path);
			if (err == AFC_E_SUCCESS){
				return 0;
			}
			return -get_afc_error_as_errno(err);
		}

		int ifuse_rename(afc_client_t afc,const char *from, const char *to){
			afc_error_t err = afc_rename_path(afc, from, to);
			if (err == AFC_E_SUCCESS){
				return 0;
			}
			return -get_afc_error_as_errno(err);
		}

		int ifuse_mkdir(afc_client_t afc,const char *dir){
			afc_error_t err = afc_make_directory(afc, dir);
			if (err == AFC_E_SUCCESS){
				return 0;
			}
			return -get_afc_error_as_errno(err);
		}
	}
}