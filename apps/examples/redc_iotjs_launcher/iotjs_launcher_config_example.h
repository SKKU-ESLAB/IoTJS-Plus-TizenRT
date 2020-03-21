/* Copyright 2019 Gyeonghwan Hong, Sungkyunkwan University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IOTJS_LAUNCHER_CONFIG_H
#define IOTJS_LAUNCHER_CONFIG_H

// This is an example of "iotjs_launcher_config_example.h".
// Please customize "iotjs_launcher_config_example.h" based on this template.

#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_SSID
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_SSID "ap_name"
#endif
#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_PASS
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_PASS "1234"
#endif
#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_AUTH
// Disable one of them
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_AUTH WIFI_MANAGER_AUTH_OPEN
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_AUTH WIFI_MANAGER_AUTH_WPA2_PSK
#endif
#ifndef CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_CRYPTO
// Disable one of them
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_CRYPTO WIFI_MANAGER_CRYPTO_NONE
#define CONFIG_EXAMPLES_IOTJS_LAUNCHER_WIFI_CRYPTO WIFI_MANAGER_CRYPTO_AES
#endif

#endif /* !defined(IOTJS_LAUNCHER_CONFIG_H) */