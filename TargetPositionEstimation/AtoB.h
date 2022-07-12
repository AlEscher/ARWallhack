#pragma once
#include <opencv/cv.h>


	
/** 
 * computes the rotation matrix from A (camera) to B (origin) as 3x3 matrix
 * @param inv result as 3x3 matrix pointer
 * @param rb rotations in x,y,z in radians as 1x3 float array
 */
void calcAtoB(Mat* inv, float* rb);
