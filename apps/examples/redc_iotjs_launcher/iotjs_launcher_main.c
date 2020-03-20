// -*- mode: C; indent-tabs-mode: t -*-
/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics France All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdio.h>
#include <libgen.h>
#include <wifi_manager/wifi_manager.h>

#include "iotjs_launcher_config.h"

#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_LAUNCH_MODE_FILE
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_LAUNCH_MODE_FILE "/mnt/launch_mode"
#endif

#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_SERVER_JS_FILE
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_SERVER_JS_FILE "/rom/iotjs_launcher_server.js"
#endif

#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_APP_JS_FILE
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_APP_JS_FILE "/mnt/index.js"
#endif

#ifndef CONFIG_EXAMPLES_IOTJS_EXTRA_MODULE_PATH
#define CONFIG_EXAMPLES_IOTJS_EXTRA_MODULE_PATH "/rom/example/iotjs_modules"
#endif

extern int iotjs(int argc, char *argv[]);

static bool g_is_connected = false;

static void iotjs_launcher_wifi_connect(void);

static void iotjs_launcher_wifi_sta_connected(wifi_manager_result_e status)
{
	printf("log: %s status=0x%x\n", __FUNCTION__, status);
	g_is_connected = ((status == WIFI_MANAGER_SUCCESS) ||
					  (status == WIFI_MANAGER_ALREADY_CONNECTED));
}

static void iotjs_launcher_wifi_sta_disconnected(wifi_manager_disconnect_e status)
{
	printf("log: %s status=0x%x\n", __FUNCTION__, status);
	g_is_connected = false;
	sleep(10);
	iotjs_launcher_wifi_connect();
}

static wifi_manager_cb_s iotjs_launcher_wifi_callbacks = {
	iotjs_launcher_wifi_sta_connected,
	iotjs_launcher_wifi_sta_disconnected,
	NULL,
	NULL,
	NULL,
};

static void iotjs_launcher_wifi_connect(void)
{
	printf("log: %s\n", __FUNCTION__);
	int i = 0;
	printf("log: Connecting to SSID \"%s\" (%d+%d)\n", CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_SSID, CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_AUTH, CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_CRYPTO);

	for (i = 0; i < 3; i++) {
		printf("log: Wait (%d/3) sec...\n", i + 1);
		sleep(1);
	}

	wifi_manager_result_e status = WIFI_MANAGER_FAIL;
	wifi_manager_ap_config_s config;

	status = wifi_manager_init(&iotjs_launcher_wifi_callbacks);
	if (status != WIFI_MANAGER_SUCCESS) {
		printf("error: wifi_manager_init (status=0x%x)\n", status);
	}
	config.ssid_length = strlen(CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_SSID);
	config.passphrase_length = strlen(CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_PASS);
	strncpy(config.ssid, CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_SSID, config.ssid_length + 1);
	strncpy(config.passphrase, CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_PASS, config.passphrase_length + 1);

	config.ap_auth_type = (wifi_manager_ap_auth_type_e)CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_AUTH;
	config.ap_crypto_type = (wifi_manager_ap_crypto_type_e)CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_CRYPTO;

	status = wifi_manager_connect_ap(&config);
	if (status != WIFI_MANAGER_SUCCESS) {
		printf("error: wifi_manager_connect_ap (status=0x%x)\n", status);
	}
}

static bool __is_launch_mode(void)
{
	FILE* fp = fopen(CONFIG_EXAMPLES_IOTJS_LAUNCHER_LAUNCH_MODE_FILE, "r");
	if (fp != NULL) {
		fclose(fp);
		return true;
	} else {
		return false;
	}
}

static void __toggle_launch_mode(void)
{
	if (__is_launch_mode()) {
		remove(CONFIG_EXAMPLES_IOTJS_LAUNCHER_LAUNCH_MODE_FILE);
	} else {
		FILE *fp = fopen(CONFIG_EXAMPLES_IOTJS_LAUNCHER_LAUNCH_MODE_FILE, "w");
		fclose(fp);
	}
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int iotjs_launcher_main(int argc, char *argv[])
#endif
{
	bool is_launch_mode = __is_launch_mode();
	__toggle_launch_mode();

	setenv("IOTJS_EXTRA_MODULE_PATH", CONFIG_EXAMPLES_IOTJS_EXTRA_MODULE_PATH, 1);
	chdir(dirname(CONFIG_EXAMPLES_IOTJS_LAUNCHER_SERVER_JS_FILE));

	if (is_launch_mode) {
		printf("\n\nIoT.js Launcher LAUNCH MODE: run %s...\n\n", CONFIG_EXAMPLES_IOTJS_LAUNCHER_APP_JS_FILE);
		char *targv[] = {"iotjs", CONFIG_EXAMPLES_IOTJS_LAUNCHER_APP_JS_FILE};
		int targc = 2;

		iotjs_launcher_wifi_connect();
		for (;;) {
			if (g_is_connected) {
				iotjs(targc, targv);
			}
			sleep(1);
		}
	} else {
		printf("\n\nIoT.js Launcher INSTALL MODE: run %s...\n\n", CONFIG_EXAMPLES_IOTJS_LAUNCHER_SERVER_JS_FILE);
		char *targv[] = {"iotjs", "--no-jmem-logs", CONFIG_EXAMPLES_IOTJS_LAUNCHER_SERVER_JS_FILE};
		int targc = 3;

		iotjs_launcher_wifi_connect();
		for (;;) {
			if (g_is_connected) {
				iotjs(targc, targv);
			}
			sleep(1);
		}
	}
	return 0;
}