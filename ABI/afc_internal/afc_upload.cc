#include "ABI/afc_internal/afc_upload.h"
#include <cstdio>
#include <dirent.h>
#include "ABI/thirdparty/glog/logging.h"

namespace afc{
	namespace internal{
		int afc_upload_file(afc_client_t afc, const char* filename, const char* dstfn){
			FILE *f = NULL;
			uint64_t af = 0;
			char buf[1024*32] = {0};
			f = fopen(filename, "rb");
			if (!f) {
				return -1;
			}
			if ((afc_file_open(afc, dstfn, AFC_FOPEN_WRONLY, &af) != AFC_E_SUCCESS) || !af) {
				fclose(f);
				LOG(ERROR)<<"afc_file_open on '"<<dstfn<<"' failed!"<<std::endl;
				return -1;
			}
			size_t amount = 0;
			do {
				amount = fread(buf, 1, sizeof(buf), f);
				if (amount > 0) {
					uint32_t written, total = 0;
					while (total < amount) {
						written = 0;
						afc_error_t aerr = afc_file_write(afc, af, buf, amount, &written);
						if (aerr != AFC_E_SUCCESS) {
							LOG(ERROR)<<"AFC Write error: "<<aerr<<std::endl;
							break;
						}
						total += written;
					}
					if (total != amount) {
						LOG(ERROR)<<"Error: wrote only "<<total<<" of "<<amount<<std::endl;
						afc_file_close(afc, af);
						fclose(f);
						return -1;
					}
				}
			} while (amount > 0);
			afc_file_close(afc, af);
			fclose(f);

			return 0;
		}
		void afc_upload_dir(afc_client_t afc, const char* path, const char* afcpath){
			afc_make_directory(afc, afcpath);
			DIR *dir = opendir(path);
			if (dir) {
				struct dirent* ep;
				while ((ep = readdir(dir))) {
					if ((strcmp(ep->d_name, ".") == 0) || (strcmp(ep->d_name, "..") == 0)) {
						continue;
					}
					char *fpath = (char*)malloc(strlen(path)+1+strlen(ep->d_name)+1);
					char *apath = (char*)malloc(strlen(afcpath)+1+strlen(ep->d_name)+1);
					struct stat st;
					strcpy(fpath, path);
					strcat(fpath, "/");
					strcat(fpath, ep->d_name);
					strcpy(apath, afcpath);
					strcat(apath, "/");
					strcat(apath, ep->d_name);
#ifdef HAVE_LSTAT
					if ((lstat(fpath, &st) == 0) && S_ISLNK(st.st_mode)) {
						char *target = (char *)malloc(st.st_size+1);
						if (readlink(fpath, target, st.st_size+1) < 0) {
							fprintf(stderr, "readlink: %s (%d)\n", strerror(errno), errno);
						} else {
							target[st.st_size] = '\0';
							afc_make_link(afc, AFC_SYMLINK, target, fpath);
						}
						free(target);
					} else
#endif
						if ((stat(fpath, &st) == 0) && S_ISDIR(st.st_mode)) {
							afc_upload_dir(afc, fpath, apath);
						} else {
							afc_upload_file(afc, fpath, apath);
						}
						free(fpath);
						free(apath);
				}
				closedir(dir);
			}
		}
	}
}
