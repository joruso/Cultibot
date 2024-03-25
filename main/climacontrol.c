/*
 * climacontrol.c
 *
 *  Created on: 29 ene 2024
 *      Author: Joruso
 */


#include "climacontrol.h"
#include "AM2302.h"

void clima_init (){
	AM2302_init(36);
}
