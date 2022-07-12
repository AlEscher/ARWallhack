#include<opencv2/opencv.hpp>
#include <cmath>

using namespace cv;

//rotations are expected to be given relative to origin in radians
void rotAtoB(Mat* inv, float* rb){
    float rx = -rb[0];
    float ry = -rb[1];
    float rz = -rb[2];
    
    //calculate individual inverse rotations
    float rotX[][3] = {{1,0,0},{0,cos(rx),-sin(rx)},{0,sin(rx),cos(rx)}};
    float rotY[][3] = {{cos(ry),0,sin(ry)},{0,1,0},{-sin(ry),0,cos(ry)}};
    float rotZ[][3]  = {{cos(rz),-sin(rz),0},{sin(rz),cos(rz),0},{0,0,1}};
    
    Mat rotMatX(Size(3,3), CV_32F, rotX);
    Mat rotMatY(Size(3,3), CV_32F, rotY);
    Mat rotMatZ(Size(3,3), CV_32F, rotZ);
    
    //calculate the combined inverse rotationn matrix
    *inv = rotMatX * rotMatY * rotMatZ;
}

//rotations are expected to be given relative to origin in meter
void transAtoB(float* coordinates, float* tb){
    float coordinates[0] += tb[0];
    float coordinates[1] += tb[1];
    float coordinates[2] += tb[2];
}

float* AtoB(float* resultMat, float* rb, float* tb){
    //coordinates relative to camera
    float* resultsA = {resultMat[3], resultMat[7], resultMat[11]};
    Mat resultAf(Size(1,3), CV_32F, resultsA);

    //calculate rotation matrix
    float invf[3][3];
    Mat inv(Size(3, 3), CV_32F, invf);
    rotAtoB(&inv, rb);
    
    //allocate coordinates to be returned
    float* coordinates = (float*)malloc(3 * sizeof(float));
    
    //coordinates after rotation but before translation
    Mat beforeTrans(Size(1,3), CV_32F, coordinates);
    beforeTrans = inv * resultAf;
    
    //translate to origin
    transAtoB(coordinates, tb);
    
    return coordinates;
}
