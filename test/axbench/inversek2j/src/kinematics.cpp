/*
 * kinematics.cpp
 * 
 *  Created on: Sep. 10 2013
 *			Author: Amir Yazdanbakhsh <yazdanbakhsh@wisc.edu>
 */

#include <cmath>
#include "kinematics.hpp"


float  __attribute((annotate("no_float 2 30 signed 0.5 0.5 0"))) l1 = 0.5 ;
float  __attribute((annotate("no_float 2 30 signed 0.5 0.5 0"))) l2 = 0.5 ;

void forwardk2j(float  __attribute((annotate("range -1.57 1.57 4e-9"))) theta1,
		float  __attribute((annotate("range -1.57 1.57 4e-9"))) theta2,
		float* __attribute((annotate("target:forwardk2j_x range -1 1 4e-9"))) x,
		float* __attribute((annotate("target:forwardk2j_y range -1 1 4e-9"))) y) {
	*x = l1 * cos(theta1) + l2 * cos(theta1 + theta2) ;
	*y = l1 * sin(theta1) + l2 * sin(theta1 + theta2) ;
}

void inversek2j(float __attribute((annotate("range -0.9 0.9 4e-9"))) x,
		float  __attribute((annotate("range -0.9 0.9 4e-9"))) y,
		float* __attribute((annotate("target:inversek2j_theta1 range -1.57 1.57 4e-9"))) theta1,
		float*  __attribute((annotate("target:inversek2j_theta2 range -1.57 1.57 4e-9"))) theta2) {

	__attribute((annotate("no_float 4 28 signed -0.9 0.9 4e-9"))) double dataIn[2];
	dataIn[0] = x;
	dataIn[1] = y;
	
	__attribute((annotate("no_float 4 28 signed -0.9 0.9 4e-9"))) double dataOut[2];

#pragma parrot(input, "inversek2j", [2]dataIn)

	*theta2 = (float)acos(((x * x) + (y * y) - (l1 * l1) - (l2 * l2))/(2 * l1 * l2));
	*theta1 = (float)asin((y * (l1 + l2 * cos(*theta2)) - x * l2 * sin(*theta2))/(x * x + y * y));

	dataOut[0] = (*theta1);
	dataOut[1] = (*theta2);

#pragma parrot(output, "inversek2j", [2]<0.0; 2.0>dataOut)


	*theta1 = dataOut[0];
	*theta2 = dataOut[1];
}
