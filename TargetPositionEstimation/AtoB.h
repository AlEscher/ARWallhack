#pragma once
#include <opencv/cv.h>


	
/** 
 * computes the rotation matrix from A (camera) to B (origin) as 3x3 matrix
 * @param inv result as 3x3 matrix pointer
 * @param rb rotations in x,y,z in radians as 1x3 float array
 */
void rotAtoB(Mat* inv, float* rb);

/**
 * computes the rotation matrix from A (camera) to B (origin) as 3x3 matrix
 * @param coordinates relative to camera as 1x3 float array to be overwritten by the result
 * @param tb translations relative to origin in x,y,z in meter as 1x3 float array
 */
void transAtoB(float* coordinates, float* tb);

/**
 * computes the coordinates relative to origin
 * @param resultMat pose matrix relative to camera
 * @param rb rotations in x,y,z in radians as 1x3 float array
 * @param tb translations relative to origin in x,y,z in meter as 1x3 float array
 * @return coordinates relative to origin
 */
float* AtoB(float* resultMat, float* rb, float* tb);
