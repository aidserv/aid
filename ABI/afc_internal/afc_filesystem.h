#ifndef ABI_AFC_INTERNAL_AFC_FILESYSTEM_H_
#define ABI_AFC_INTERNAL_AFC_FILESYSTEM_H_
//////////////////////////////////////////////////////////////////////////
#include <fcntl.h>
#include <libimobiledevice/afc.h>
//////////////////////////////////////////////////////////////////////////
namespace afc{
	namespace internal{
		class timeval{
		public:
			long    tv_sec;
			long    tv_usec;
		};
		struct StatAttr{
			int	st_mode;        //文件对应的模式，文件、目录等
			ino_t	st_ino;            //inode节点号
			dev_t	st_dev;           //设备号码
			dev_t	st_rdev;          //特殊设备号码
			unsigned long	st_nlink;         //文件的连接数
			unsigned long	st_uid;            //文件所有者
			unsigned long	st_gid;            //文件所有者对应的组
			off_t	st_size;          //普通文件对应的文件字节数
			time_t	st_atime;        //文件最后被访问的时间
			time_t	st_mtime;       //文件内容最后被修改的时间
			time_t	st_ctime;        //文件状态改变时间
			time_t	st_birthtime;
			size_t	st_blksize;     //文件内容对应原块大小
			size_t	st_blocks;      //文件内容对应的块数量
		};
		struct StatByte{
			uint64_t totalspace;
			uint64_t freespace;
			uint64_t blocksize;
		};
		int ifuse_getattr(afc_client_t afc,const char *path,struct StatAttr* stbuf);
		int ifuse_readdir(afc_client_t afc,const char *path,char ***dirs = NULL);
		int ifuse_open(afc_client_t afc,const char *path,uint64_t* fi,int flag = O_RDONLY);
		int ifuse_create(afc_client_t afc,const char *path,uint64_t *fi,int flag = O_RDONLY);
		int ifuse_read(afc_client_t afc, char *buf, size_t size, off_t offset,uint64_t fi);
		int ifuse_write(afc_client_t afc,const char *path, const char *buf, size_t size, off_t offset,uint64_t fi);
		int ifuse_utimens(afc_client_t afc,const char *path, const timeval tv[2]);
		int ifuse_release(afc_client_t afc,uint64_t fi);
		int ifuse_statfs(afc_client_t afc,const char *path,StatByte* bytes);
		int ifuse_truncate(afc_client_t afc,const char *path, off_t size);
		int ifuse_ftruncate(afc_client_t afc,const char *path, off_t size,uint64_t fi);
		int ifuse_symlink(afc_client_t afc,const char *target, const char *linkname);
		int ifuse_link(afc_client_t afc,const char *target, const char *linkname);
		int ifuse_unlink(afc_client_t afc,const char *path);
		int ifuse_rename(afc_client_t afc,const char *from, const char *to);
		int ifuse_mkdir(afc_client_t afc,const char *dir);
	}
}
//////////////////////////////////////////////////////////////////////////
#endif