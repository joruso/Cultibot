
/*
 * server.h
 *
 *  Created on: 23 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_SERVER_H_
#define MAIN_SERVER_H_

#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_server.h>

void start_webserver(void);

void stop_webserver(void);

#endif /* MAIN_SERVER_H_ */
