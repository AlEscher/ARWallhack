#include<opencv2/opencv.hpp>
#include <cmath>

using namespace cv;

//rotations are expected to be given relative to origin in radians
//coordinatesInB = inv * coordinatesInA
void calcAtoB(Mat* inv, float* rb){
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
