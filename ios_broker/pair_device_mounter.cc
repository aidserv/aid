#include "ios_broker/pair_device_mounter.h"
#define _GNU_SOURCE 1
#define __USE_GNU 1
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <getopt.h>
#include <cerrno>
#include <libgen.h>
#include <time.h>
#include <inttypes.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/notification_proxy.h>
#include <asprintf.h>
#include <libimobiledevice/mobile_image_mounter.h>
#include <libimobiledevice/libimobiledevice/common/utils.h>
#include "ABI/thirdparty/glog/logging.h"

static const char PKG_PATH[] = "PublicStaging";
static const char PATH_PREFIX[] = "/private/var/mobile/Media";

typedef enum {
	DISK_IMAGE_UPLOAD_TYPE_AFC,
	DISK_IMAGE_UPLOAD_TYPE_UPLOAD_IMAGE
} disk_image_upload_type_t;


namespace ios_broker{
	size_t mim_upload_cb(void* buf, size_t size, void* userdata){
		return fread(buf, 1, size, (FILE*)userdata);
	}
	void message(const char* format,...){
		char buffer[1024] = {0};
		va_list args;
		va_start(args,format);
		vsnprintf(buffer,1024,format,args);
		LOG(INFO)<<buffer<<std::endl;
		va_end (args);
	}
	int Mounter(bool list_mode,internal::PairConnectioned* connected,const char* image_path,const char* image_sig_path,const char* imagetype){
		lockdownd_error_t ldret = LOCKDOWN_E_UNKNOWN_ERROR;
		mobile_image_mounter_client_t mim = NULL;
		afc_client_t afc = NULL;
		lockdownd_service_descriptor_t service = NULL;
		int res = -1;
		size_t image_size = 0;
		plist_t pver = NULL;
		char *product_version = NULL;
		lockdownd_get_value(connected->client_handle(), NULL, "ProductVersion", &pver);
		if (pver && plist_get_node_type(pver) == PLIST_STRING) {
			plist_get_string_val(pver, &product_version);
		}
		if(pver){
			plist_free(pver);
		}
		disk_image_upload_type_t disk_image_upload_type = DISK_IMAGE_UPLOAD_TYPE_AFC;
		int product_version_major = 0;
		int product_version_minor = 0;
		if (product_version) {
			if (sscanf(product_version, "%d.%d.%*d", &product_version_major, &product_version_minor) == 2) {
				if (product_version_major >= 7)
					disk_image_upload_type = DISK_IMAGE_UPLOAD_TYPE_UPLOAD_IMAGE;
			}
		}

		lockdownd_start_service(connected->client_handle(), "com.apple.mobile.mobile_image_mounter", &service);

		if (!service || service->port == 0) {
			message("ERROR: Could not start mobile_image_mounter service!\n");
			goto leave;
		}

		if (mobile_image_mounter_new(connected->device_handle(), service, &mim) != MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
			message("ERROR: Could not connect to mobile_image_mounter!\n");
			goto leave;
		}	

		if (service) {
			lockdownd_service_descriptor_free(service);
			service = NULL;
		}

		if (!list_mode) {
			struct stat fst;
			if (disk_image_upload_type == DISK_IMAGE_UPLOAD_TYPE_AFC) {
				if ((lockdownd_start_service(connected->client_handle(), "com.apple.afc", &service) !=
					LOCKDOWN_E_SUCCESS) || !service || !service->port) {
						goto leave;
				}
				if (afc_client_new(connected->device_handle(), service, &afc) != AFC_E_SUCCESS) {
					goto leave;
				}
				if (service) {
					lockdownd_service_descriptor_free(service);
					service = NULL;
				}
			}
			if (stat(image_path, &fst) != 0) {
				fprintf(stderr, "ERROR: stat: %s: %s\n", image_path, strerror(errno));
				goto leave;
			}
			image_size = fst.st_size;
			if (stat(image_sig_path, &fst) != 0) {
				fprintf(stderr, "ERROR: stat: %s: %s\n", image_sig_path, strerror(errno));
				goto leave;
			}
		}

		mobile_image_mounter_error_t err;
		plist_t result = NULL;

		if (list_mode) {
			/* list mounts mode */
			if (!imagetype) {
				imagetype = strdup("Developer");
			}
			err = mobile_image_mounter_lookup_image(mim, imagetype, &result);
			if (err == MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
				res = 0;
				plist_print_to_stream(result, stdout);
			} else {
				message("Error: lookup_image returned %d\n", err);
			}
		} else {
			char sig[8192];
			size_t sig_length = 0;
			FILE *f = fopen(image_sig_path, "rb");
			if (!f) {
				fprintf(stderr, "Error opening signature file '%s': %s\n", image_sig_path, strerror(errno));
				goto leave;
			}
			sig_length = fread(sig, 1, sizeof(sig), f);
			fclose(f);
			if (sig_length == 0) {
				fprintf(stderr, "Could not read signature from file '%s'\n", image_sig_path);
				goto leave;
			}

			f = fopen(image_path, "rb");
			if (!f) {
				fprintf(stderr, "Error opening image file '%s': %s\n", image_path, strerror(errno));
				goto leave;
			}

			char *targetname = NULL;
			if (asprintf(&targetname, "%s/%s", PKG_PATH, "staging.dimage") < 0) {
				fprintf(stderr, "Out of memory!?\n");
				goto leave;
			}
			char *mountname = NULL;
			if (asprintf(&mountname, "%s/%s", PATH_PREFIX, targetname) < 0) {
				fprintf(stderr, "Out of memory!?\n");
				goto leave;
			}

			if (!imagetype) {
				imagetype = strdup("Developer");
			}

			switch(disk_image_upload_type) {
			case DISK_IMAGE_UPLOAD_TYPE_UPLOAD_IMAGE:
				message("Uploading %s\n", image_path);
				err = mobile_image_mounter_upload_image(mim, imagetype, image_size, sig, sig_length, mim_upload_cb, f);
				break;
			case DISK_IMAGE_UPLOAD_TYPE_AFC:
			default:
				message("Uploading %s --> afc:///%s\n", image_path, targetname);
				char **strs = NULL;
				if (afc_get_file_info(afc, PKG_PATH, &strs) != AFC_E_SUCCESS) {
					if (afc_make_directory(afc, PKG_PATH) != AFC_E_SUCCESS) {
						fprintf(stderr, "WARNING: Could not create directory '%s' on device!\n", PKG_PATH);
					}
				}
				if (strs) {
					int i = 0;
					while (strs[i]) {
						free(strs[i]);
						i++;
					}
					free(strs);
				}

				uint64_t af = 0;
				if ((afc_file_open(afc, targetname, AFC_FOPEN_WRONLY, &af) !=
					AFC_E_SUCCESS) || !af) {
						
						fclose(f);
						fprintf(stderr, "afc_file_open on '%s' failed!\n", targetname);
						if(targetname){
							free(targetname);
						}
						if(mountname){
							free(mountname);
						}
						if(imagetype){
							free((void*)imagetype);
						}
						goto leave;
				}

				char buf[8192];
				size_t amount = 0;
				do {
					amount = fread(buf, 1, sizeof(buf), f);
					if (amount > 0) {
						uint32_t written, total = 0;
						while (total < amount) {
							written = 0;
							if (afc_file_write(afc, af, buf, amount, &written) !=
								AFC_E_SUCCESS) {
									fprintf(stderr, "AFC Write error!\n");
									break;
							}
							total += written;
						}
						if (total != amount) {
							fprintf(stderr, "Error: wrote only %d of %d\n", total,
								(unsigned int)amount);
							afc_file_close(afc, af);
							fclose(f);
							goto leave;
						}
					}
				}
				while (amount > 0);

				afc_file_close(afc, af);
				break;
			}

			fclose(f);

			message("done.\n");

			message("Mounting...\n");
			err = mobile_image_mounter_mount_image(mim, mountname, sig, sig_length, imagetype, &result);
			if (err == MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
				if (result) {
					plist_t node = plist_dict_get_item(result, "Status");
					if (node) {
						char *status = NULL;
						plist_get_string_val(node, &status);
						if (status) {
							if (!strcmp(status, "Complete")) {
								message("Done.\n");
								res = 0;
							} else {
								message("unexpected status value:\n");
								plist_print_to_stream(result, stdout);
							}
							free(status);
						} else {
							message("unexpected result:\n");
							plist_print_to_stream(result, stdout);
						}
					}
					node = plist_dict_get_item(result, "Error");
					if (node) {
						char *error = NULL;
						plist_get_string_val(node, &error);
						if (error) {
							message("Error: %s\n", error);
							free(error);
						} else {
							message("unexpected result:\n");
							//plist_print_to_stream(result, stdout);
						}

					}
				}
			} else {
				message("Error: mount_image returned %d\n", err);

			}
			if(targetname){
				free(targetname);
			}
			if(mountname){
				free(mountname);
			}
			if(imagetype){
				free((void*)imagetype);
			}
		}

		if (result) {
			plist_free(result);
		}

		/* perform hangup command */
		mobile_image_mounter_hangup(mim);
		/* free client */
		mobile_image_mounter_free(mim);

leave:
		if (service) {
			lockdownd_service_descriptor_free(service);
			service = NULL;
		}
		if (afc) {
			afc_client_free(afc);
		}
		if(product_version){
			free(product_version);
		}
		return res;
	}
}