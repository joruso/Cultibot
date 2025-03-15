/*
 * sntp.h
 *
 *  Created on: 21 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_SNTP_H_
#define MAIN_SNTP_H_

#define SNTP_TIME_SERVER "pool.ntp.org"


#include <time.h>
#include <esp_err.h>

esp_err_t obtain_time();

#endif /* MAIN_SNTP_H_ */
