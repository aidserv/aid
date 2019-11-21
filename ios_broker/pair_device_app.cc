/**
 * ideviceinstaller -- Manage iPhone/iPod apps
 *
 * Copyright (C) 2010 Nikias Bassen <nikias@gmx.li>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more profile.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 
 * USA
 */

#ifndef __func__
# define __func__ __FUNCTION__
#endif

// #ifndef S_ISREG
// # define S_ISREG(B) ((B)&_S_IFREG)
// #endif
// #ifndef S_ISDIR
// # define S_ISDIR(B) ((B)&_S_IFDIR)
// #endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#define _GNU_SOURCE 1
#define __USE_GNU 1
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include <inttypes.h>
#include <limits.h>
#include <dirent.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/notification_proxy.h>
#include <libimobiledevice/afc.h>
#include <plist/plist.h>
#include <libzip/zip.h>
#include "ios_broker/pair_device_app.h"
#include "ABI/afc_internal/afc_upload.h"
#include "PhoneCore/interface/unque_pointer.h"
#include "ABI/base/lock_mutex.h"
#include "ABI/zip_internal/zip_internal_impl.h"
#include "ABI/thirdparty/glog/logging.h"

#ifdef _WIN32
# include <windows.h>
# define strdup _strdup
#endif

#include <asprintf.h>

#ifndef PRIu64
# define PRIu64 "llu"
#endif
#ifndef PRIi64
# define PRIi64 "ll"
#endif

const char PKG_PATH[] = "PublicStaging";
const char APPARCH_PATH[] = "ApplicationArchives";

char *last_status = NULL;
int wait_for_op_complete = 0;
int notification_expected = 0;
int op_completed = 0;
int err_occured = 0;
int notified = 0;

namespace ios_broker{
	namespace internal{
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
		static void notifier(const char *notification, void *unused)
#else
		static void notifier(const char *notification)
#endif
		{
			/*message("notification received: %s\n", notification);*/
			notified = 1;
		}
		static void status_cb(plist_t command, plist_t status, void *unused){
			if (status && command) {
				plist_t npercent = plist_dict_get_item(status, "PercentComplete");
				plist_t nstatus = plist_dict_get_item(status, "Status");
				plist_t nerror = plist_dict_get_item(status, "Error");
				int percent = 0;
				char *status_msg = NULL;
				if (npercent) {
					uint64_t val = 0;
					plist_get_uint_val(npercent, &val);
					percent = val;
				}
				if (nstatus) {
					plist_get_string_val(nstatus, &status_msg);
					if (!strcmp(status_msg, "Complete")){
						wait_for_op_complete = 0;
						notification_expected = 0;
						op_completed = 1;
					}
				}
				if(!nerror) {
					if (!npercent) {
						sdk::pointer::GetInstance()->get_interface()->BrokerProgressCallback((const char*)command, status_msg,0);
					} 
					else {
						sdk::pointer::GetInstance()->get_interface()->BrokerProgressCallback((const char*)command, status_msg, percent);
					}
				} 
				else {
					char *err_msg = NULL;
					plist_get_string_val(nerror, &err_msg);
					sdk::pointer::GetInstance()->get_interface()->BrokerErrorMessage(err_msg);
					LOG(ERROR)<<(const char*)command<<" - Error occured: "<<err_msg<<std::endl;
					free(err_msg);
					err_occured = 1;
					wait_for_op_complete = 0;
					notification_expected = 0;
				}
				if (last_status) {
					free(last_status);
					last_status = NULL;
				}
				if (status_msg) {
					last_status = strdup(status_msg);
					free(status_msg);
				}
			} else {
				LOG(ERROR)<<__func__<<": called with invalid data!"<<std::endl;
			}
		}
		static void do_wait_when_needed()
		{
			int i = 0;
#ifndef _WIN32
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 500000000;
#else
			unsigned long sleep_duration = 500;
#endif
			/* wait for operation to complete */
			while (wait_for_op_complete && !op_completed && !err_occured
				&& !notified && (i < 60)) {
#ifndef _WIN32
					nanosleep(&ts, NULL);
#else
					Sleep(sleep_duration);
#endif
					i++;
			}

			/* wait some time if a notification is expected */
			while (notification_expected && !notified && !err_occured && (i < 10)) {
#ifndef _WIN32
				nanosleep(&ts, NULL);
#else
				Sleep(sleep_duration);
#endif
				i++;
			}
		}
	}
	 PairDeviceApp::PairDeviceApp(){
		InitializeBroker();
	}
	 PairDeviceApp::~PairDeviceApp(){
		 InitializeBroker();
	}
	void PairDeviceApp::SetDebugLavel(){
		idevice_set_debug_level(1);
	}
	void PairDeviceApp::set_list_apps_mode(int status){
		list_apps_mode_ = status;
	}
	void PairDeviceApp::set_install_mode(int status){
		install_mode_ = status;
	}
	void PairDeviceApp::set_uninstall_mode(int status){
		uninstall_mode_ = status;
	}
	void PairDeviceApp::set_upgrade_mode(int status){
		upgrade_mode_ = status;
	}
	void PairDeviceApp::set_list_archives_mode(int status){
		list_archives_mode_ = status;
	}
	void PairDeviceApp::set_archive_mode(int status){
		archive_mode_ = status;
	}
	void PairDeviceApp::set_restore_mode(int status){
		restore_mode_ = status;
	}
	void PairDeviceApp::set_remove_archive_mode(int status){
		remove_archive_mode_ = status;
	}
	void PairDeviceApp::InitializeBroker(){
		if (last_status) {
			free(last_status);
			last_status = NULL;
		}
		wait_for_op_complete = 0;
		notification_expected = 0;
		op_completed = 0;
		err_occured = 0;
		notified = 0;
		set_list_apps_mode(0);
		set_install_mode(0);
		set_uninstall_mode(0);
		set_upgrade_mode(0);
		set_list_archives_mode(0);
		set_archive_mode(0);
		set_restore_mode(0);
		set_remove_archive_mode(0);
	}
	int PairDeviceApp::InstallBroker(const char* udid,const char* appid,const char* options){
		idevice_t phone = NULL;
		lockdownd_client_t client = NULL;
		instproxy_client_t ipc = NULL;
		np_client_t np = NULL;
		afc_client_t afc = NULL;
		lockdownd_service_descriptor_t service_descriptor = NULL;
		char* bundleidentifier = NULL;
		int res = -1;
		if (IDEVICE_E_SUCCESS != idevice_new(&phone,udid)){
			LOG(ERROR)<<"No iPhone found, is it plugged in?"<<std::endl;
			return -1;
		}
		if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(phone,&client,"ideviceinstaller")){
			LOG(ERROR)<<"Could not connect to lockdownd. Exiting."<<std::endl;
			goto leave_cleanup;
		}
		if ((lockdownd_start_service(client, "com.apple.mobile.notification_proxy",&service_descriptor) != LOCKDOWN_E_SUCCESS) || !service_descriptor->port){
			LOG(ERROR)<<"Could not start com.apple.mobile.notification_proxy!"<<std::endl;
			goto leave_cleanup;
		}
		if (np_client_new(phone, service_descriptor, &np) != NP_E_SUCCESS){
			LOG(ERROR)<<"Could not connect to notification_proxy!"<<std::endl;
			goto leave_cleanup;
		}
		if (service_descriptor) {
			lockdownd_service_descriptor_free(service_descriptor);
			service_descriptor = NULL;
		}
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
		np_set_notify_callback(np, internal::notifier, NULL);
#else
		np_set_notify_callback(np, internal::notifier);
#endif

		const char *noties[3] = { NP_APP_INSTALLED, NP_APP_UNINSTALLED, NULL };

		np_observe_notifications(np, noties);

run_again:
		service_descriptor = NULL;
		if ((lockdownd_start_service(client, "com.apple.mobile.installation_proxy",&service_descriptor) != LOCKDOWN_E_SUCCESS) || !service_descriptor->port) {
			LOG(ERROR)<<"Could not start com.apple.mobile.installation_proxy!"<<std::endl;
			goto leave_cleanup;
		}

		if (instproxy_client_new(phone, service_descriptor, &ipc) != INSTPROXY_E_SUCCESS) {
			LOG(ERROR)<<"Could not connect to installation_proxy!"<<std::endl;
			goto leave_cleanup;
		}
		if(last_status){
			free(last_status);
			last_status = NULL;
		}
		notification_expected = 0;

		if (list_apps_mode_) {
			int xml_mode = 0;
			plist_t client_opts = instproxy_client_options_new();
			instproxy_client_options_add(client_opts, "ApplicationType", "User", NULL);
			instproxy_error_t err;
			plist_t apps = NULL;

			/* look for options */
			if (options) {
				char *opts = strdup(options);
				char *elem = strtok(opts,",");
				while (elem) {
					if (!strcmp(elem, "list_system")) {
						if (!client_opts) {
							client_opts = instproxy_client_options_new();
						}
						instproxy_client_options_add(client_opts, "ApplicationType", "System", NULL);
					} else if (!strcmp(elem, "list_all")) {
						instproxy_client_options_free(client_opts);
						client_opts = NULL;
					} else if (!strcmp(elem, "list_user")) {
						/* do nothing, we're already set */
					} else if (!strcmp(elem, "xml")) {
						xml_mode = 1;
					}
					elem = strtok(NULL, ",");
				}
				free(opts);
			}
			if (!xml_mode) {
				//references:https://github.com/libimobiledevice/ideviceinstaller/blob/master/src/ideviceinstaller.c
				instproxy_client_options_set_return_attributes(client_opts,
					"CFBundleIdentifier",
					"CFBundleDisplayName",
					"CFBundleVersion",
					"StaticDiskUsage",
					"DynamicDiskUsage",
					NULL);
			}
			err = instproxy_browse(ipc, client_opts, &apps);
			//err = instproxy_browse_with_callback(ipc, client_opts, status_cb, NULL);
			if (err == INSTPROXY_E_RECEIVE_TIMEOUT) {
				LOG(ERROR) << "NOTE: timeout waiting for device to browse apps, trying again..." << err << std::endl;
				goto leave_cleanup;
			}
			instproxy_client_options_free(client_opts);
			if (err != INSTPROXY_E_SUCCESS) {
				LOG(ERROR)<<"ERROR: instproxy_browse returned "<<err<<std::endl;
				goto leave_cleanup;
			}
			if (!apps || (plist_get_node_type(apps) != PLIST_ARRAY)) {
				LOG(ERROR)<<"ERROR: instproxy_browse returnd an invalid plist!"<<std::endl;
				goto leave_cleanup;
			}
			if (xml_mode) {
				char *xml = NULL;
				uint32_t len = 0;
				plist_to_xml(apps, &xml, &len);
				if (xml) {
					puts(xml);
					free(xml);
				}
				plist_free(apps);
				goto leave_cleanup;
			}
			//message("Total: %d apps\n", plist_array_get_size(apps));
			sdk::pointer::GetInstance()->ibroker_nterface()->AppTotal(plist_array_get_size(apps));
			uint32_t i = 0;
			for (i = 0; i < plist_array_get_size(apps); i++) {
				char* xml_buf = NULL;
				size_t xml_len = 0;
				char* s_appid = NULL;
				plist_t app = plist_array_get_item(apps, i);
				plist_to_xml(app,&xml_buf,&xml_len);
#ifdef _DEBUG
				LOG(ERROR) << xml_buf << std::endl;
#endif
				plist_t p_appid = plist_dict_get_item(app, "CFBundleIdentifier");
				if(p_appid) {
					plist_get_string_val(p_appid, &s_appid);
				}
				uint64_t dynamic_disk_usage = 0;
				p_appid = plist_dict_get_item(app, "DynamicDiskUsage");
				if (p_appid) {
					plist_get_uint_val(p_appid, &dynamic_disk_usage);
				}
				uint64_t static_disk_usage = 0;
				p_appid = plist_dict_get_item(app, "StaticDiskUsage");
				if (p_appid) {
					plist_get_uint_val(p_appid, &static_disk_usage);
				}
				sdk::pointer::GetInstance()->ibroker_nterface()->AppDetail(udid,s_appid,xml_buf,dynamic_disk_usage,static_disk_usage);
				free(xml_buf);
				free(s_appid);
			}
			plist_free(apps);
		} 
		else if(install_mode_ || upgrade_mode_){
			plist_t sinf = NULL;
			plist_t meta = NULL;
			char *pkgname = NULL;
			struct stat fst;
			FILE *f = NULL;
			uint64_t af = 0;
			char buf[8192];
			if(service_descriptor){
				lockdownd_service_descriptor_free(service_descriptor);
				service_descriptor = NULL;
			}
			if((lockdownd_start_service(client,"com.apple.afc",&service_descriptor) != LOCKDOWN_E_SUCCESS) || !service_descriptor->port){
				LOG(ERROR)<<"Could not start com.apple.afc!"<<std::endl;
				goto leave_cleanup;
			}
			lockdownd_client_free(client);
			client = NULL;
			if (afc_client_new(phone,service_descriptor,&afc) != INSTPROXY_E_SUCCESS){
				LOG(ERROR)<<"Could not connect to AFC!"<<std::endl;
				goto leave_cleanup;
			}

			if (stat(appid, &fst) != 0) {
				LOG(ERROR)<<"stat: "<<appid<<": "<<strerror(errno)<<std::endl;
				goto leave_cleanup;
			}
			char **strs = NULL;
			if (afc_get_file_info(afc, PKG_PATH, &strs) != AFC_E_SUCCESS) {
				if (afc_make_directory(afc, PKG_PATH) != AFC_E_SUCCESS) {
					LOG(WARNING)<<"Could not create directory '"<<PKG_PATH<<"' on device!"<<std::endl;
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
			/* open install package */
			int errp = 0;
			struct zip *zf = zip_open(appid, 0, &errp);
			if (!zf) {
				LOG(ERROR)<<"zip_open: "<<appid<<": "<<errp<<std::endl;
				goto leave_cleanup;
			}

			plist_t client_opts = instproxy_client_options_new();

			if ((strlen(appid) > 5) && (strcmp(&appid[strlen(appid)-5], ".ipcc") == 0)) {
				char* ipcc = strdup(appid);
				if ((asprintf(&pkgname, "%s/%s", PKG_PATH, basename(ipcc)) > 0) && pkgname) {
					afc_make_directory(afc, pkgname);
				}

				message("Uploading %s package contents...\n", basename(ipcc));

				/* extract the contents of the .ipcc file to PublicStaging/<name>.ipcc directory */
				zip_uint64_t numzf = zip_get_num_entries(zf, 0);
				zip_uint64_t i = 0;
				for (i = 0; numzf > 0 && i < numzf; i++) {
					const char* zname = zip_get_name(zf, i, 0);
					char* dstpath = NULL;
					if (!zname) continue;
					if (zname[strlen(zname)-1] == '/') {
						// directory
						if ((asprintf(&dstpath, "%s/%s/%s", PKG_PATH, basename(ipcc), zname) > 0) && dstpath) {
							afc_make_directory(afc, dstpath);						}
						free(dstpath);
						dstpath = NULL;
					} else {
						// file
						struct zip_file* zfile = zip_fopen_index(zf, i, 0);
						if (!zfile) continue;

						if ((asprintf(&dstpath, "%s/%s/%s", PKG_PATH, basename(ipcc), zname) <= 0) || !dstpath || (afc_file_open(afc, dstpath, AFC_FOPEN_WRONLY, &af) != AFC_E_SUCCESS)) {
							LOG(ERROR)<<"can't open afc://"<<dstpath<<" for writing"<<std::endl;
							free(dstpath);
							dstpath = NULL;
							zip_fclose(zfile);
							continue;
						}

						struct zip_stat zs;
						zip_stat_init(&zs);
						if (zip_stat_index(zf, i, 0, &zs) != 0) {
							LOG(ERROR)<<"zip_stat_index %" <<i<< " failed!"<<std::endl;
							free(dstpath);
							dstpath = NULL;
							zip_fclose(zfile);
							continue;
						}

						free(dstpath);
						dstpath = NULL;

						zip_uint64_t zfsize = 0;
						while (zfsize < zs.size) {
							zip_int64_t amount = zip_fread(zfile, buf, sizeof(buf));
							if (amount == 0) {
								break;
							}

							if (amount > 0) {
								uint32_t written, total = 0;
								while (total < amount) {
									written = 0;
									if (afc_file_write(afc, af, buf, amount, &written) != AFC_E_SUCCESS) {
										LOG(ERROR)<<"AFC Write error!"<<std::endl;
											break;
									}
									total += written;
								}
								if (total != amount) {
									LOG(ERROR)<<"Error: wrote only "<<total<<" of %" <<amount<<std::endl;
									instproxy_client_options_free(client_opts);
									afc_file_close(afc, af);
									zip_fclose(zfile);
									free(dstpath);
									goto leave_cleanup;
								}
							}

							zfsize += amount;
						}

						afc_file_close(afc, af);
						af = 0;

						zip_fclose(zfile);
					}
				}
				free(ipcc);
				message("done.\n");

				instproxy_client_options_add(client_opts, "PackageType", "CarrierBundle", NULL);
				instproxy_client_options_free(client_opts);
			} 
			else if (S_ISDIR(fst.st_mode)){
				instproxy_client_options_add(client_opts, "PackageType", "Developer", NULL);
				if (asprintf(&pkgname, "%s/%s", PKG_PATH, basename((char*)appid)) < 0) {
					LOG(ERROR)<<"Out of memory allocating pkgname!?"<<std::endl;
					goto leave_cleanup;
				}
				message("Uploading %s package contents... ", basename((char*)appid));
				afc::internal::afc_upload_dir(afc, appid, pkgname);
				instproxy_client_options_free(client_opts);
				message("DONE.\n");
			}
			else {
				/* extract iTunesMetadata.plist from package */
				char *zbuf = NULL;
				uint32_t len = 0;
				plist_t meta_dict = NULL;
				if (ABI::zip_internal::zip_get_contents(zf, "iTunesMetadata.plist", 0, &zbuf, &len) == 0) {
					meta = plist_new_data(zbuf, len);
					if (memcmp(zbuf, "bplist00", 8) == 0) {
						plist_from_bin(zbuf, len, &meta_dict);
					} else {
						plist_from_xml(zbuf, len, &meta_dict);
					}
				}
				if (zbuf) {
					free(zbuf);
				}

				/* determine .app directory in archive */
				zbuf = NULL;
				len = 0;
				plist_t info = NULL;
				char filename[256];
				filename[0] = '\0';
				char* app_directory_name = NULL;

				if(ABI::zip_internal::zip_get_app_directory(zf, &app_directory_name)) {
					LOG(ERROR)<<"Unable to locate app directory in archive!"<<std::endl;
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					goto leave_cleanup;
				}

				/* construct full filename to Info.plist */
				strcpy(filename, app_directory_name);
				free(app_directory_name);
				app_directory_name = NULL;
				strcat(filename, "Info.plist");

				if (ABI::zip_internal::zip_get_contents(zf, filename, 0, &zbuf, &len) < 0) {
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					zip_unchange_all(zf);
					zip_close(zf);
					goto leave_cleanup;
				}
				if (memcmp(zbuf, "bplist00", 8) == 0) {
					plist_from_bin(zbuf, len, &info);
				} else {
					plist_from_xml(zbuf, len, &info);
				}
				free(zbuf);

				if (!info) {
					LOG(ERROR)<<"Could not parse Info.plist!"<<std::endl;
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(info);
					plist_free(meta);
					zip_unchange_all(zf);
					zip_close(zf);
					goto leave_cleanup;
				}

				char *bundleexecutable = NULL;

				plist_t bname = plist_dict_get_item(info, "CFBundleExecutable");
				if (bname) {
					plist_get_string_val(bname, &bundleexecutable);
				}
				bname = plist_dict_get_item(info, "CFBundleIdentifier");
				if (bname) {
					plist_get_string_val(bname, &bundleidentifier);
				}
				plist_free(info);
				info = NULL;

				if (!bundleexecutable) {
					LOG(ERROR)<<"Could not determine value for CFBundleExecutable!"<<std::endl;
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					zip_unchange_all(zf);
					zip_close(zf);
					goto leave_cleanup;
				}

				char *sinfname = NULL;
				if (asprintf(&sinfname, "Payload/%s.app/SC_Info/%s.sinf", bundleexecutable, bundleexecutable) < 0) {
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					LOG(ERROR)<<"Out of memory!?"<<std::endl;
					goto leave_cleanup;
				}
				free(bundleexecutable);

				/* extract .sinf from package */
				zbuf = NULL;
				len = 0;
				if (ABI::zip_internal::zip_get_contents(zf, sinfname, 0, &zbuf, &len) == 0) {
					sinf = plist_new_data(zbuf, len);
				}
				free(sinfname);
				if (zbuf) {
					free(zbuf);
				}

				/* copy archive to device */
				f = fopen(appid, "rb");
				if (!f) {
					LOG(ERROR)<<"fopen: "<<appid<<": "<<strerror(errno)<<std::endl;
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					plist_free(sinf);
					goto leave_cleanup;
				}

				pkgname = NULL;
				if (asprintf(&pkgname, "%s/%s", PKG_PATH, bundleidentifier/*basename(const_cast<char*>(appid))*/) < 0) {
					LOG(ERROR)<<"Out of memory!?"<<std::endl;
					fclose(f);
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					plist_free(sinf);
					goto leave_cleanup;
				}
				if (afc::internal::afc_upload_file(afc, appid, pkgname) < 0) {
					free(pkgname);
					instproxy_client_options_free(client_opts);
					plist_free(meta_dict);
					plist_free(meta);
					plist_free(sinf);
					goto leave_cleanup;
				}
				message("Copying '%s' --> '%s'\n", appid, pkgname);
				message("done.\n");
				if (bundleidentifier) {
					instproxy_client_options_add(client_opts, "CFBundleIdentifier", bundleidentifier, NULL);
				}
				if (sinf) {
					instproxy_client_options_add(client_opts, "ApplicationSINF", sinf, NULL);
				}
				if (meta) {
					instproxy_client_options_add(client_opts, "iTunesMetadata", meta, NULL);
				}
				plist_free(meta_dict);
				plist_free(meta);
				plist_free(sinf);
			}
			zip_unchange_all(zf);
			zip_close(zf);

			/* perform installation or upgrade */
			if (install_mode_) {
				message("Installing '%s'\n", pkgname);
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
				instproxy_install(ipc, pkgname, client_opts, (instproxy_status_cb_t)internal::status_cb, NULL);
#else
				instproxy_install(ipc, pkgname, client_opts, internal::status_cb);
#endif
			} else {
				message("Upgrading '%s'\n", pkgname);
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
				instproxy_upgrade(ipc, pkgname, client_opts, internal::status_cb, NULL);
#else
				instproxy_upgrade(ipc, pkgname, client_opts, internal::status_cb);
#endif
			}
			instproxy_client_options_free(client_opts);
			free(pkgname);
			wait_for_op_complete = 1;
			notification_expected = 1;
		} else if (uninstall_mode_) {
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
			instproxy_uninstall(ipc, appid, NULL, internal::status_cb, NULL);
#else
			instproxy_uninstall(ipc, appid, NULL, internal::status_cb);
#endif
			wait_for_op_complete = 1;
			notification_expected = 1;
		} else if (list_archives_mode_) {
			int xml_mode = 0;
			plist_t dict = NULL;
			plist_t lres = NULL;
			instproxy_error_t err;

			/* look for options */
			if (options) {
				char *opts = strdup(options);
				char *elem = strtok(opts,",");
				while (elem) {
					if (!strcmp(elem, "xml")) {
						xml_mode = 1;
					}
					elem = strtok(NULL, ",");
				}
				free(opts);
			}

			err = instproxy_lookup_archives(ipc, NULL, &dict);
			if (err != INSTPROXY_E_SUCCESS) {
				LOG(ERROR)<<"ERROR: lookup_archives returned "<<err<<std::endl;
				goto leave_cleanup;
			}
			if (!dict) {
				LOG(ERROR)<<"ERROR: lookup_archives did not return a plist!?"<<std::endl;
				goto leave_cleanup;
			}

			lres = plist_dict_get_item(dict, "LookupResult");
			if (!lres || (plist_get_node_type(lres) != PLIST_DICT)) {
				plist_free(dict);
				LOG(ERROR)<<"ERROR: Could not get dict 'LookupResult'"<<std::endl;
				goto leave_cleanup;
			}

			if (xml_mode) {
				char *xml = NULL;
				uint32_t len = 0;

				plist_to_xml(lres, &xml, &len);
				if (xml) {
					puts(xml);
					free(xml);
				}
				plist_free(dict);
				goto leave_cleanup;
			}
			plist_dict_iter iter = NULL;
			plist_t node = NULL;
			char *key = NULL;

			//message("Total: %d archived apps\n", plist_dict_get_size(lres));
			sdk::pointer::GetInstance()->ibroker_nterface()->AppTotal(plist_dict_get_size(lres));
			plist_dict_new_iter(lres, &iter);
			if (!iter) {
				plist_free(dict);
				LOG(ERROR)<<"ERROR: Could not create plist_dict_iter!"<<std::endl;
				goto leave_cleanup;
			}
			do {
				key = NULL;
				node = NULL;
				plist_dict_next_item(lres, iter, &key, &node);
				if (key && (plist_get_node_type(node) == PLIST_DICT)) {
					char* xml_buf = NULL;
					size_t xml_len = 0;
					plist_to_xml(node,&xml_buf,&xml_len);
					sdk::pointer::GetInstance()->ibroker_nterface()->AppDetail(udid,appid,xml_buf,0,0);
					free(xml_buf);
				}
			}while (node);
			plist_free(dict);
		} else if (archive_mode_) {
			char *copy_path = NULL;
			int remove_after_copy = 0;
			int skip_uninstall = 1;
			int app_only = 0;
			int docs_only = 0;
			plist_t client_opts = NULL;

			/* look for options */
			if (options) {
				char *opts = strdup(options);
				char *elem = strtok(opts,",");
				while (elem) {
					if (!strcmp(elem, "uninstall")) {
						skip_uninstall = 0;
					} else if (!strcmp(elem, "app_only")) {
						app_only = 1;
						docs_only = 0;
					} else if (!strcmp(elem, "docs_only")) {
						docs_only = 1;
						app_only = 0;
					} else if ((strlen(elem) > 5) && !strncmp(elem, "copy=", 5)) {
						copy_path = strdup(elem+5);
					} else if (!strcmp(elem, "remove")) {
						remove_after_copy = 1;
					}
					elem = strtok(NULL, ",");
				}
				free(opts);
			}

			if (skip_uninstall || app_only || docs_only) {
				client_opts = instproxy_client_options_new();
				if (skip_uninstall) {
					instproxy_client_options_add(client_opts, "SkipUninstall", 1, NULL);
				}
				if (app_only) {
					instproxy_client_options_add(client_opts, "ArchiveType", "ApplicationOnly", NULL);
				} else if (docs_only) {
					instproxy_client_options_add(client_opts, "ArchiveType", "DocumentsOnly", NULL);
				}
			}

			if (copy_path) {
				struct stat fst;
				if (stat(copy_path, &fst) != 0) {
					LOG(ERROR)<<"stat: "<<copy_path<<": "<<strerror(errno)<<std::endl;
					free(copy_path);
					goto leave_cleanup;
				}

				if (!S_ISDIR(fst.st_mode)) {
					LOG(ERROR)<<copy_path<<" is not a directory as expected."<<std::endl;
					free(copy_path);
					goto leave_cleanup;
				}

				service_descriptor = NULL;
				if ((lockdownd_start_service(client, "com.apple.afc", &service_descriptor) != LOCKDOWN_E_SUCCESS) || !service_descriptor->port) {
					LOG(ERROR)<<"Could not start com.apple.afc!"<<std::endl;
					free(copy_path);
					goto leave_cleanup;
				}

				lockdownd_client_free(client);
				client = NULL;

				if (afc_client_new(phone, service_descriptor, &afc) != INSTPROXY_E_SUCCESS) {
					LOG(ERROR)<<"Could not connect to AFC!"<<std::endl;
					goto leave_cleanup;
				}
			}

#ifdef HAVE_LIBIMOBILEDEVICE_1_1
			instproxy_archive(ipc, appid, client_opts, internal::status_cb, NULL);
#else
			instproxy_archive(ipc, appid, client_opts, internal::status_cb);
#endif
			instproxy_client_options_free(client_opts);
			wait_for_op_complete = 1;
			if (skip_uninstall) {
				notification_expected = 0;
			} else {
				notification_expected = 1;
			}

			internal::do_wait_when_needed();

			if (copy_path) {
				if (err_occured) {
					afc_client_free(afc);
					afc = NULL;
					goto leave_cleanup;
				}
				FILE *f = NULL;
				uint64_t af = 0;
				/* local filename */
				char *localfile = NULL;
				if (asprintf(&localfile, "%s/%s.ipa", copy_path, appid) < 0) {
					LOG(ERROR)<<"Out of memory!?"<<std::endl;
					goto leave_cleanup;
				}
				free(copy_path);

				f = fopen(localfile, "wb");
				if (!f) {
					LOG(ERROR)<<"fopen: "<<localfile<<": "<<strerror(errno)<<std::endl;
					free(localfile);
					goto leave_cleanup;
				}

				/* remote filename */
				char *remotefile = NULL;
				if (asprintf(&remotefile, "%s/%s.zip", APPARCH_PATH, appid) < 0) {
					LOG(ERROR)<<"Out of memory!?"<<std::endl;
					goto leave_cleanup;
				}

				uint32_t fsize = 0;
				char **fileinfo = NULL;
				if ((afc_get_file_info(afc, remotefile, &fileinfo) != AFC_E_SUCCESS) || !fileinfo) {
					LOG(ERROR)<<"getting AFC file info for '"<<remotefile<<"' on device!"<<std::endl;
					fclose(f);
					free(remotefile);
					free(localfile);
					goto leave_cleanup;
				}

				int i;
				for (i = 0; fileinfo[i]; i+=2) {
					if (!strcmp(fileinfo[i], "st_size")) {
						fsize = atoi(fileinfo[i+1]);
						break;
					}
				}
				i = 0;
				while (fileinfo[i]) {
					free(fileinfo[i]);
					i++;
				}
				free(fileinfo);

				if (fsize == 0) {
					LOG(ERROR)<<"Hm... remote file length could not be determined. Cannot copy."<<std::endl;
					fclose(f);
					free(remotefile);
					free(localfile);
					goto leave_cleanup;
				}

				if ((afc_file_open(afc, remotefile, AFC_FOPEN_RDONLY, &af) != AFC_E_SUCCESS) || !af) {
					fclose(f);
					LOG(ERROR)<<"could not open '"<<remotefile<<"' on device for reading!"<<std::endl;
					free(remotefile);
					free(localfile);
					goto leave_cleanup;
				}

				/* copy file over */
				message("Copying '%s' --> '%s'\n", remotefile, localfile);
				free(remotefile);
				free(localfile);

				uint32_t amount = 0;
				uint32_t total = 0;
				char buf[8192];

				do {
					if (afc_file_read(afc, af, buf, sizeof(buf), &amount) != AFC_E_SUCCESS) {
						LOG(ERROR)<<"AFC Read error!"<<std::endl;
						break;
					}

					if (amount > 0) {
						size_t written = fwrite(buf, 1, amount, f);
						if (written != amount) {
							LOG(ERROR)<<"Error when writing "<<amount<<" bytes to local file!"<<std::endl;
							break;
						}
						total += written;
					}
				} while (amount > 0);

				afc_file_close(afc, af);
				fclose(f);

				message("done.\n");
				if (total != fsize) {
					LOG(WARNING)<<"remote and local file sizes don't match ("<<fsize<<"!="<<total<<")"<<std::endl;
					if (remove_after_copy) {
						LOG(ERROR)<<"archive file will NOT be removed from device"<<std::endl;
						remove_after_copy = 0;
					}
				}

				if (remove_after_copy) {
					/* remove archive if requested */
					message("Removing '%s'\n", appid);
					set_archive_mode(0);
					set_remove_archive_mode(1);
					if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(phone, &client, "ideviceinstaller")) {
						LOG(ERROR)<<"Could not connect to lockdownd. Exiting."<<std::endl;
						goto leave_cleanup;
					}
					goto run_again;
				}
			}
			goto leave_cleanup;
		} else if (restore_mode_) {
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
			instproxy_restore(ipc, appid, NULL, internal::status_cb, NULL);
#else
			instproxy_restore(ipc, appid, NULL, internal::status_cb);
#endif
			wait_for_op_complete = 1;
			notification_expected = 1;
		} else if (remove_archive_mode_) {
#ifdef HAVE_LIBIMOBILEDEVICE_1_1
			instproxy_remove_archive(ipc, appid, NULL, internal::status_cb, NULL);
#else
			instproxy_remove_archive(ipc, appid, NULL, internal::status_cb);
#endif
			wait_for_op_complete = 1;
		}
		else {
			LOG(ERROR)<<"no operation selected?! This should not be reached!"<<std::endl;
			res = -2;
			goto leave_cleanup;
		}

		if (client) {
			/* not needed anymore */
			lockdownd_client_free(client);
			client = NULL;
		}
		internal::do_wait_when_needed();
		res = 0;
leave_cleanup:
		if (bundleidentifier) {
			free(bundleidentifier);
		}
		if (np) {
			np_client_free(np);
		}
		if (ipc) {
			instproxy_client_free(ipc);
		}
		if (afc) {
			afc_client_free(afc);
		}
		if (client) {
			lockdownd_client_free(client);
		}
		if (service_descriptor) {
			lockdownd_service_descriptor_free(service_descriptor);
			service_descriptor = NULL;
		}
		idevice_free(phone);
		if (err_occured){
			res = -1;
		}
		InitializeBroker();
		return res;
	}
	PairDeviceApp* PairDeviceApp::GetInstance(){
		static PairDeviceApp* info;
		if(!info){
			PairDeviceApp* new_info = new PairDeviceApp;
			if(InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&info),new_info,NULL)){
				delete new_info;
			}
		}
		return info;
	}
	void PairDeviceApp::message(const char* format,...){
		char buffer[1024] = {0};
		va_list args;
		va_start(args,format);
		vsnprintf(buffer,1024,format,args);
		LOG(INFO)<<buffer<<std::endl;
		va_end (args);
	}
}
