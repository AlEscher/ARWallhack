#include<opencv2/opencv.hpp>
#include<iostream>
#include<string.h>
#include <cmath>
#include"PoseEstimation.h"
#include "json.hpp"
#include "AsyncCommunicationServer.h"
// getopt is a POSIX function thus not available on Windows. 
// For Windows plebs like me I needed some stolen recompiled c version of getops :D
#if defined(_WIN32)
#include "Win_Getopt.h"
#else
#include <getopt.h>
#endif


using namespace cv;
using namespace std;


#define EX5 1
#define EX5_RAW 0
#define DRAW_CONTOUR 0
#define DRAW_RECTANGLE 0

#define THICKNESS_VALUE 4


// Struct holding all infos about each strip, e.g. length
struct MyStrip {
    int stripeLength;
    int nStop;
    int nStart;
    Point2f stripeVecX;
    Point2f stripeVecY;
};

int sock = 0, valread, client_fd;
struct sockaddr_in serv_addr;
// List of points
typedef vector<Point> contour_t;
// List of contours
typedef vector<contour_t> contour_vector_t;


const int threshold_slider_max = 255;
int threshold_slider = 0;

const int fps = 30;

Mat videoStreamFrameGray;
Mat videoStreamFrameOutput;

const string stripWindow = "Strip Window";

const std::string kWinName4 = "Exercise 4 - Marker";

// JSON String that is sent to all Clients
std::string POS_MESSAGE;

// Added in sheet 4 - Start *****************************************************************

int bw_thresh = 55;

// Added in sheet 4 - End *******************************************************************

// Pos is from UI, dereferencing of the pointer
static void on_trackbar(int pos, void* slider_value) {
    *((int*)slider_value) = pos;
    // C++ >= 11 -> Standard
    //*(static_cast<int*>(slider_value)) = pos;
}

// Added in sheet 4 - Start *****************************************************************

void bw_trackbarHandler(int pos, void* slider_value) {
    *((int*)slider_value) = pos;
}

// Added in sheet 4 - End *******************************************************************

int subpixSampleSafe(const Mat& pSrc, const Point2f& p) {
    // Point is float, slide 14
    int fx = int(floorf(p.x));
    int fy = int(floorf(p.y));

    if (fx < 0 || fx >= pSrc.cols - 1 ||
        fy < 0 || fy >= pSrc.rows - 1)
        return 127;

    // Slides 15
    int px = int(256 * (p.x - floorf(p.x)));
    int py = int(256 * (p.y - floorf(p.y)));

    // Here we get the pixel of the starting point
    unsigned char* i = (unsigned char*)((pSrc.data + fy * pSrc.step) + fx);

    // Internsity, shift 3
    int a = i[0] + ((px * (i[1] - i[0])) >> 8);
    i += pSrc.step;
    int b = i[0] + ((px * (i[1] - i[0])) >> 8);

    // We want to return Intensity for the subpixel
    return a + ((py * (b - a)) >> 8);
}

// Added in Sheet 3 - Ex7 (a) Start *****************************************************************
Mat calculate_Stripe(double dx, double dy, MyStrip & st) {
    // Norm (euclidean distance) from the direction vector is the length (derived from the Pythagoras Theorem)
    double diffLength = sqrt(dx * dx + dy * dy);

    // Length proportional to the marker size
    st.stripeLength = (int)(0.8 * diffLength);

    if (st.stripeLength < 5)
        st.stripeLength = 5;

    // Make stripeLength odd (because of the shift in nStop), Example 6: both sides of the strip must have the same length XXXOXXX
    st.stripeLength |= 1;
    /*if (st.stripeLength % 2 == 0)
        st.stripeLength++;*/

    // E.g. stripeLength = 5 --> from -2 to 2: Shift -> half top, the other half bottom
    st.nStop = st.stripeLength >> 1;
    //st.nStop = st.stripeLength / 2;
    st.nStart = -st.nStop;

    Size stripeSize;

    // Sample a strip of width 3 pixels
    stripeSize.width = 3;
    stripeSize.height = st.stripeLength;

    // Normalized direction vector
    st.stripeVecX.x = dx / diffLength;
    st.stripeVecX.y = dy / diffLength;

    // Normalized perpendicular vector
    st.stripeVecY.x = st.stripeVecX.y;
    st.stripeVecY.y = -st.stripeVecX.x;

    // 8 bit unsigned char with 1 channel, gray
    return Mat(stripeSize, CV_8UC1);
}

static void getCoordinates(float* coordinates, float* resultMatrix, cv::Point2f* corners, float markerSize, float* tb, float* rb, bool verbose = false){
    estimateSquarePose(resultMatrix, corners, markerSize);

    // This part is only for printing
    if(verbose){
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            cout << setw(6); // Total 6
            cout << setprecision(4); // Numbers of decimal places = 4 (of the 6)
            // Again we are going through a matrix which is saved as an array
            cout << resultMatrix[4 * i + j] << " ";
        }
        cout << "\n";
    }
    }
    cout << "\n";
    float x, y, z;
    // Translation values in the transformation matrix to calculate the distance between the marker and the camera
    x = resultMatrix[3];
    y = resultMatrix[7];
    z = resultMatrix[11];
    
    float rx, ry, rz = 0;
    
    ry = -atan(resultMatrix[8]);
    rx = atan2(resultMatrix[9]/cos(ry) , resultMatrix[10]/cos(ry));
    rz = atan2(resultMatrix[4]/cos(ry) , resultMatrix[0]/cos(ry));

    float rotX[][3] = {{1,0,0},{0,cos(rb[0]),-sin(rb[0])},{0,sin(rb[0]),cos(rb[0])}};
    float rotY[][3] = {{cos(rb[1]),0,sin(rb[1])},{0,1,0},{-sin(rb[1]),0,cos(rb[1])}};
    float rotZ[][3]  = {{cos(rb[2]),-sin(rb[2]),0},{sin(rb[2]),cos(rb[2]),0},{0,0,1}};
    float rotf[3][3];
    
    Mat rotMatX(Size(3,3), CV_32F, rotX);
    Mat rotMatY(Size(3,3), CV_32F, rotY);
    Mat rotMatZ(Size(3,3), CV_32F, rotZ);
    Mat rot(Size(3,3), CV_32F, rotf);
    rot = rotMatZ * rotMatY * rotMatX;
    rot = rot.inv();
    
    float distf[3] = {x,y,z};
    Mat dist(Size(1,3), CV_32F, distf);

    float rxb, ryb, rzb = 0;
    
    ryb = -atan(rotf[2][0]);
    rxb = atan2(rotf[2][1]/cos(ryb) , rotf[2][2]/cos(ryb));
    rzb = atan2(rotf[1][0]/cos(ryb) , rotf[0][0]/cos(ryb));

    Mat distB(Size(1,3), CV_32F, coordinates);
    distB = rot * dist;
    coordinates[0] += tb[0];
    coordinates[1] += tb[1];
    coordinates[2] += tb[2];
    
    if(verbose){
    cout <<"rotations in x, y , z (A): " << rx << " " << ry << " " << rz << "\n";
    cout <<"rotations in x, y , z (B): " << rxb << " " << ryb << " " << rzb << "\n";
    cout <<"coordinates : " << coordinates[0] << " ; " << coordinates[1] << " ; " << coordinates[2] << "\n";

    /* TASK: How can we calculate the distance? -> HINT: E... */
    cout << "length: " << sqrt(x*x + y*y + z*z) << "\n";
    cout << "\n";
    cout << "dist to B: " << sqrt(coordinates[0] * coordinates[0] + coordinates[1] * coordinates[1] + coordinates[2] * coordinates[2]) << "\n";
    }
}

static void contourToMarker(Point2f** cornersR, contour_vector_t contours, Mat *imgFiltered, Mat *grayScale, int k, bool isFirstMarker, bool isFirstStripe, int* codeR){
    // --- Process Contour ---
    Point2f nul;
    nul.x = -1;
    nul.y = -1;
    contour_t approx_contour;

    // Simplifying of the contour with the Ramer-Douglas-Peuker Algorithm
    // true -> Only closed contours
    // Approximation of old curve, the difference (epsilon) should not be bigger than: perimeter(->arcLength)*0.02
    approxPolyDP(contours[k], approx_contour, arcLength(contours[k], true) * 0.02, true);

    Scalar QUADRILATERAL_COLOR(0, 0, 255);
    Scalar colour;
    // Convert to a usable rectangle
    Rect r = boundingRect(approx_contour);

    // 4 Corners -> We color them
    if (approx_contour.size() == 4) {
        colour = QUADRILATERAL_COLOR;
    }
    else {
        *cornersR = NULL;
        return;
    }

    // --- Filter tiny ones --- If the found contour is too small (20 -> pixels, frame.cols - 10 to prevent extreme big contours)
    if (r.height < 20 || r.width < 20 || r.width > imgFiltered->cols - 10 || r.height > imgFiltered->rows - 10) {
        *cornersR = NULL;
        return;
    }
    // -> Cleaning done

    // 1 -> 1 contour, we have a closed contour, true -> closed, 4 -> thickness
    polylines(*imgFiltered, approx_contour, true, colour, THICKNESS_VALUE);

    // Direction vector (x0,y0) and contained point (x1,y1) -> For each line -> 4x4 = 16
    float lineParams[16];
    // lineParams is shared, CV_32F -> Same data type like lineParams
    Mat lineParamsMat(Size(4, 4), CV_32F, lineParams);

    // --- Process Corners ---

    for (size_t i = 0; i < approx_contour.size(); ++i) {
        // Render the corners, 3 -> Radius, -1 filled circle
        circle(*imgFiltered, approx_contour[i], 3, CV_RGB(0, 255, 0), -1);

        // Euclidic distance, 7 -> parts, both directions dx and dy
        double dx = ((double)approx_contour[(i + 1) % 4].x - (double)approx_contour[i].x) / 7.0;
        double dy = ((double)approx_contour[(i + 1) % 4].y - (double)approx_contour[i].y) / 7.0;

        MyStrip strip;

        // A simple array of unsigned char cv::Mat
        Mat imagePixelStripe = calculate_Stripe(dx, dy, strip);

        // Array for edge point centers
        Point2f edgePointCenters[6];

        // First point already rendered, now the other 6 points
        for (int j = 1; j < 7; ++j) {
            // Position calculation
            double px = (double)approx_contour[i].x + (double)j * dx;
            double py = (double)approx_contour[i].y + (double)j * dy;

            Point p;
            p.x = (int)px;
            p.y = (int)py;
            circle(*imgFiltered, p, 2, CV_RGB(0, 0, 255), -1);

            // Columns: Loop over 3 pixels
            for (int m = -1; m <= 1; ++m) {
                // Rows: From bottom to top of the stripe, e.g. -3 to 3
                for (int n = strip.nStart; n <= strip.nStop; ++n) {
                    Point2f subPixel;

                    // m -> going over the 3 pixel thickness of the stripe, n -> over the length of the stripe, direction comes from the orthogonal vector in st
                    // Going from bottom to top and defining the pixel coordinate for each pixel belonging to the stripe
                    subPixel.x = (double)p.x + ((double)m * strip.stripeVecX.x) + ((double)n * strip.stripeVecY.x);
                    subPixel.y = (double)p.y + ((double)m * strip.stripeVecX.y) + ((double)n * strip.stripeVecY.y);

                    Point p2;
                    p2.x = (int)subPixel.x;
                    p2.y = (int)subPixel.y;

                    // The one (purple color) which is shown in the stripe window
                    if (isFirstStripe)
                        circle(*imgFiltered, p2, 1, CV_RGB(255, 0, 255), -1);
                    else
                        circle(*imgFiltered, p2, 1, CV_RGB(0, 255, 255), -1);

                    // Combined Intensity of the subpixel
                    int pixelIntensity = subpixSampleSafe(*grayScale, subPixel);

                    // Converte from index to pixel coordinate
                    // m (Column, real) -> -1,0,1 but we need to map to 0,1,2 -> add 1 to 0..2
                    int w = m + 1;

                    // n (Row, real) -> add stripeLenght >> 1 to shift to 0..stripeLength
                    // n=0 -> -length/2, n=length/2 -> 0 ........ + length/2
                    int h = n + (strip.stripeLength >> 1);

                    // Set pointer to correct position and safe subpixel intensity
                    imagePixelStripe.at<uchar>(h, w) = (uchar)pixelIntensity;

                    // Added in Sheet 3 - Ex7 (a) End *****************************************************************
                }
            }
            // Use sobel operator on stripe
            // The first and last row must be excluded from the sobel calculation because they have no top or bottom neighbors
            vector<double> sobelValues(strip.stripeLength - 2.);

            // To use the kernel we start with the second row (n) and stop before the last one
            for (int n = 1; n < (strip.stripeLength - 1); n++) {
                // Take the intensity value from the stripe
                unsigned char* stripePtr = &(imagePixelStripe.at<uchar>(n - 1, 0));

                // Calculation of the gradient with the sobel for the first row
                double r1 = -stripePtr[0] - 2. * stripePtr[1] - stripePtr[2];

                // Go two lines for the third line of the sobel, step = size of the data type, here uchar
                stripePtr += 2 * imagePixelStripe.step;

                // Calculation of the gradient with the sobel for the third row
                double r3 = stripePtr[0] + 2. * stripePtr[1] + stripePtr[2];

                // Writing the result into our sobel value vector
                unsigned int ti = n - 1;
                sobelValues[ti] = r1 + r3;
            }

            double maxIntensity = -1;
            int maxIntensityIndex = 0;

            // Finding the max value
            for (int n = 0; n < strip.stripeLength - 2; ++n) {
                if (sobelValues[n] > maxIntensity) {
                    maxIntensity = sobelValues[n];
                    maxIntensityIndex = n;
                }
            }

            // f(x) slide 7 -> y0 .. y1 .. y2
            double y0, y1, y2;

            // Point before and after
            unsigned int max1 = maxIntensityIndex - 1, max2 = maxIntensityIndex + 1;

            // If the index is at the border we are out of the stripe, then we will take 0
            y0 = (maxIntensityIndex <= 0) ? 0 : sobelValues[max1];
            y1 = sobelValues[maxIntensityIndex];
            // If we are going out of the array of the sobel values
            y2 = (maxIntensityIndex >= strip.stripeLength - 3) ? 0 : sobelValues[max2];

            // Formula for calculating the x-coordinate of the vertex of a parabola, given 3 points with equal distances
            // (xv means the x value of the vertex, d the distance between the points):
            // xv = x1 + (d / 2) * (y2 - y0)/(2*y1 - y0 - y2)

            // Equation system
            // d = 1 because of the normalization and x1 will be added later
            double pos = (y2 - y0) / (4 * y1 - 2 * y0 - 2 * y2);

            // What happens when there is no solution? -> /0 or Number = other Number
            // If the found pos is not a number -> there is no solution
            if (isnan(pos)) {
                continue;
            }
            // Check if there is a solution to the calculation, cool trick
            /*if (pos != pos) {
                // Value is not a number -> NAN
                continue;
            }*/

            // Exact point with subpixel accuracy
            Point2d edgeCenter;

            // Back to Index positioning, Where is the edge (max gradient) in the picture?
            int maxIndexShift = maxIntensityIndex - (strip.stripeLength >> 1);

            // Shift the original edgepoint accordingly -> Is the pixel point at the top or bottom?
            edgeCenter.x = (double)p.x + (((double)maxIndexShift + pos) * strip.stripeVecY.x);
            edgeCenter.y = (double)p.y + (((double)maxIndexShift + pos) * strip.stripeVecY.y);

            // Highlight the subpixel with blue color
            circle(*imgFiltered, edgeCenter, 2, CV_RGB(0, 0, 255), -1);

            edgePointCenters[j - 1].x = edgeCenter.x;
            edgePointCenters[j - 1].y = edgeCenter.y;
            
            // Draw the stripe in the image
            if (isFirstStripe) {
                Mat iplTmp;
                // The intensity differences on the stripe
                resize(imagePixelStripe, iplTmp, Size(100, 300));

                imshow(stripWindow, iplTmp);
                isFirstStripe = false;
            }
        }

        // We now have the array of exact edge centers stored in "points", every row has two values -> 2 channels!
        Mat highIntensityPoints(Size(1, 6), CV_32FC2, edgePointCenters);

        // fitLine stores the calculated line in lineParams per column in the following way:
        // vec.x, vec.y, point.x, point.y
        // Norm 2, 0 and 0.01 -> Optimal parameters
        // i -> Edge points
        fitLine(highIntensityPoints, lineParamsMat.col(i), CV_DIST_L2, 0, 0.01, 0.01);
        // We need two points to draw the line
        Point p1;
        // We have to jump through the 4x4 matrix, meaning the next value for the wanted line is in the next row -> +4
        // d = -50 is the scalar -> Length of the line, g: Point + d*Vector
        // p1<----Middle---->p2
        //   <-----100----->
        p1.x = (int)lineParams[8 + i] - (int)(50.0 * lineParams[i]);
        p1.y = (int)lineParams[12 + i] - (int)(50.0 * lineParams[4 + i]);

        Point p2;
        p2.x = (int)lineParams[8 + i] + (int)(50.0 * lineParams[i]);
        p2.y = (int)lineParams[12 + i] + (int)(50.0 * lineParams[4 + i]);

        // Draw line
        line(*imgFiltered, p1, p2, CV_RGB(0, 255, 255), 1, 8, 0);
    }

    // So far we stored the exact line parameters and show the lines in the image now we have to calculate the exact corners
    Point2f* corners = *cornersR;

    // Calculate the intersection points of both lines
    for (int i = 0; i < 4; ++i) {
        // Go through the corners of the rectangle, 3 -> 0
        int j = (i + 1) % 4;

        double x0, x1, y0, y1, u0, u1, v0, v1;

        // We have to jump through the 4x4 matrix, meaning the next value for the wanted line is in the next row -> +4
        // g: Point + d*Vector
        // g1 = (x0,y0) + scalar0*(u0,v0) == g2 = (x1,y1) + scalar1*(u1,v1)
        x0 = lineParams[i + 8]; y0 = lineParams[i + 12];
        x1 = lineParams[j + 8]; y1 = lineParams[j + 12];

        // Direction vector
        u0 = lineParams[i]; v0 = lineParams[i + 4];
        u1 = lineParams[j]; v1 = lineParams[j + 4];

        // Cramer's rule
        // 2 unknown a,b -> Equation system
        double a = x1 * u0 * v1 - y1 * u0 * u1 - x0 * u1 * v0 + y0 * u0 * u1;
        double b = -x0 * v0 * v1 + y0 * u0 * v1 + x1 * v0 * v1 - y1 * v0 * u1;

        // Calculate the cross product to check if both direction vectors are parallel -> = 0
        // c -> Determinant = 0 -> linear dependent -> the direction vectors are parallel -> No division with 0
        double c = v1 * u0 - v0 * u1;
        if (fabs(c) < 0.001) {
            std::cout << "lines parallel" << std::endl;
            continue;
        }

        // We have checked for parallelism of the direction vectors
        // -> Cramer's rule, now divide through the main determinant
        a /= c;
        b /= c;

        // Exact corner
        corners[i].x = a;
        corners[i].y = b;

        // Added in sheet 4 Ex9 (b)- End *******************************************************************

        Point p;
        p.x = (int)corners[i].x;
        p.y = (int)corners[i].y;

        // Added in sheet 4 Ex9 (c)- Start *****************************************************************

        circle(*imgFiltered, p, 5, CV_RGB(255, 255, 0), -1);

        // Added in sheet 4 Ex9 (c)- End *******************************************************************

    } // End of the loop to extract the exact corners

    // Coordinates on the original marker images to go to the actual center of the first pixel -> 6x6
    Point2f targetCorners[4];
    targetCorners[0].x = -0.5; targetCorners[0].y = -0.5;
    targetCorners[1].x = 5.5; targetCorners[1].y = -0.5;
    targetCorners[2].x = 5.5; targetCorners[2].y = 5.5;
    targetCorners[3].x = -0.5; targetCorners[3].y = 5.5;

    // Create and calculate the matrix of perspective transform -> non-affine -> parallel stays not parallel
    // Homography is a matrix to describe the transformation from an image region to the 2D projected image
    Mat homographyMatrix(Size(3, 3), CV_32FC1);
    // Corner which we calculated and our target Mat, find the transformation
    homographyMatrix = getPerspectiveTransform(corners, targetCorners);

    // Added in sheet 4 Ex10 (a)- End *******************************************************************

    // Added in sheet 4 Ex10 (b)- Start *****************************************************************

    // Create image for the marker
    Mat imageMarker(Size(6, 6), CV_8UC1);

    // Change the perspective in the marker image using the previously calculated Homography Matrix
    // In the Homography Matrix there is also the position in the image saved
    warpPerspective(*grayScale, imageMarker, homographyMatrix, Size(6, 6));

    // Added in sheet 4 Ex10 (b)- End *******************************************************************

    // Now we have a B/W image of a supposed Marker
    threshold(imageMarker, imageMarker, bw_thresh, 255, CV_THRESH_BINARY);

    // Added in sheet 4 Ex10 (c)- Start *****************************************************************

    int code = 0;
    for (int i = 0; i < 6; ++i) {
        // Check if border is black
        int pixel1 = imageMarker.at<uchar>(0, i); //top
        int pixel2 = imageMarker.at<uchar>(5, i); //bottom
        int pixel3 = imageMarker.at<uchar>(i, 0); //left
        int pixel4 = imageMarker.at<uchar>(i, 5); //right

        // 0 -> black
        if ((pixel1 > 0) || (pixel2 > 0) || (pixel3 > 0) || (pixel4 > 0)) {
            code = -1;
            break;
        }
    }

    if (code < 0) {
        *cornersR = nullptr;
        return;
    }

    // Copy the BW values into cP -> codePixel on the marker 4x4 (inner part of the marker, no black border)
    int cP[4][4];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            // +1 -> no borders!
            cP[i][j] = imageMarker.at<uchar>(i + 1, j + 1);
            // If black then 1 else 0
            cP[i][j] = (cP[i][j] == 0) ? 1 : 0;
        }
    }

    // Save the ID of the marker, for each side
    int codes[4];
    codes[0] = codes[1] = codes[2] = codes[3] = 0;

    // Calculate the code from all sides in one loop
    for (int i = 0; i < 16; i++) {
        // /4 to go through the rows
        int row = i >> 2;
        int col = i % 4;

        // Multiplied by 2 to check for black values -> 0*2 = 0
        codes[0] <<= 1;
        codes[0] |= cP[row][col]; // 0∞

        // 4x4 structure -> Each column represents one side
        codes[1] <<= 1;
        codes[1] |= cP[3 - col][row]; // 90∞

        codes[2] <<= 1;
        codes[2] |= cP[3 - row][3 - col]; // 180∞

        codes[3] <<= 1;
        codes[3] |= cP[col][3 - row]; // 270∞
    }

    // Account for symmetry -> One side complete white or black
    if ((codes[0] == 0) || (codes[0] == 0xffff)) {
        *cornersR = nullptr;
        return;
    }

    int angle = 0;

    // Search for the smallest marker ID
    code = codes[0];
    for (int i = 1; i < 4; ++i) {
        if (codes[i] < code) {
            code = codes[i];
            angle = i;
        }
    }
    //cout << "code: " << code << "\n";
    // Print ID
    //printf("Found: %04x\n", code);

    // Show the first detected marker in the image
    if (isFirstMarker) {
        imshow(kWinName4, imageMarker);
        isFirstMarker = false;
    }

    // Correct the order of the corners, if 0 -> already have the 0 degree position
    /* TASK: How to make sure, that we always have the correct order? */
    if (angle != 0) {
        Point2f corrected_corners[4];
        // Smallest id represents the x-axis, we put the values in the corrected_corners array
        for (int i = 0; i < 4; i++)    corrected_corners[(i + angle) % 4] = corners[i];
        // We put the values back in the array in the sorted order
        for (int i = 0; i < 4; i++)    corners[i] = corrected_corners[i];
    }

    // Normally we should do a camera calibration to get the camera paramters such as focal length
    // Two ways: Inner parameters, e.g. focal length (intrinsic parameters); camera with 6 dof (R|T) (extrinsic parameters)
    // Transfer screen coords to camera coords -> To get to the principal point
    for (int i = 0; i < 4; i++) {
        // Here you have to use your own camera resolution (x) * 0.5
        corners[i].x -= 540;
        // -(corners.y) -> is needed because y is inverted
        // Here you have to use your own camera resolution (y) * 0.5
        corners[i].y = -corners[i].y + 360;
    }
    *codeR = code;
}


int main(int argc, char *argv[]) {
    float tb[] = {0,0,0};
    float rb[] = {0,0,0};
    
    
    char opt;
    while((opt = getopt(argc, argv, "t:r:h")) != -1){
        switch (opt) {
            case 't': {
                int offs = 0;
                for(int i = 0; i < 3; i++){
                    tb[i] = atof(optarg + offs);
                    while(*(optarg + offs) != ',' && i < 2){
                        if(*(optarg + offs) == '\0'){
                            cout << "use -h to see usage";
                            return -1;
                        }
                        offs++;
                    }
                    offs++;
                }
            }
                break;
            case 'r': {
                int offs = 0;
                for(int i = 0; i < 3; i++){
                    rb[i] = atof(optarg + offs);
                    while(rb[i] >= 3.14159265359)
                        rb[i] -= 3.14159265359;
                    while(*(optarg + offs) != ',' && i < 2){
                        if(*(optarg + offs) == '\0'){
                            cout << "use -h to see usage";
                            return -1;
                        }
                        offs++;
                    }
                    offs++;
                }
            }
                break;
            case 'h':
                cout << "use -t xt,yt,zt to pass camera coortdinates\nuse -r xr,yr,zr to pass rotation\nuse -h for help";
                return 0;
            default:
                break;
        }
    }
    

    // setup server acceptor loop in own thread
    boost::asio::io_context io_context;
    tcp_server server(io_context);
    std::thread my_thread([&]() { io_context.run(); });



    Mat frame;
    VideoCapture cap(0);

    const string streamWindow = "Stream";

    if (!cap.isOpened()) {
        return -1;
    }

    bool isFirstStripe = true;

    bool isFirstMarker = true;

    const string contoursWindow = "Contours";
    const string UI = "Threshold";
    namedWindow(contoursWindow, CV_WINDOW_AUTOSIZE);
    //namedWindow(stripWindow, CV_WINDOW_AUTOSIZE);

    int slider_value = 100;
    createTrackbar(UI, contoursWindow, &slider_value, 255, on_trackbar, &slider_value);

    //int bw_sileder_value = bw_thresh;

    //createTrackbar("BW Threshold", contoursWindow, &bw_sileder_value, 255, bw_trackbarHandler, &bw_sileder_value);

    namedWindow(kWinName4, CV_WINDOW_NORMAL);

    resizeWindow(kWinName4, 120, 120);

    Mat imgFiltered;
    float targets[10][3];
    int codes[10];
    int ind = 0;
    while (cap.read(frame)) {
        ind = 0;
        // --- Process Frame ---

        Mat grayScale;
        imgFiltered = frame.clone();
        cvtColor(imgFiltered, grayScale, COLOR_BGR2GRAY);

        // Threshold to reduce the noise
        threshold(grayScale, grayScale, slider_value, 255, THRESH_BINARY);

        contour_vector_t contours;

        // RETR_LIST is a list of all found contour, SIMPLE is to just save the begin and ending of each edge which belongs to the contour
        findContours(grayScale, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

        //drawContours(imgFiltered, contours, -1, Scalar(0, 255, 0), 4, 1);

        auto networkCoordinates = nlohmann::json::array();
        // size is always positive, so unsigned int -> size_t; if you have not initialized the vector it is -1, hence crash
        for (size_t k = 0; k < contours.size(); k++) {
            Point2f* corners = (Point2f*)malloc(4 * sizeof(Point2f));
            int* code  = (int*)malloc(sizeof(int));
            float* resultMatrix = (float*)malloc(16 * sizeof(float));
            contourToMarker(&corners, contours, &imgFiltered, &grayScale, k, isFirstMarker, isFirstStripe, code);
            // 4x4 -> Rotation | Translation
            //        0  0  0  | 1 -> (Homogene coordinates to combine rotation, translation and scaling)
            if(corners != NULL) {
	            // Marker size in meters!
	            getCoordinates(targets[ind], resultMatrix, corners, 0.03, tb, rb);
	            codes[ind] = *code;
                nlohmann::json coordinate;
                coordinate["id"] = codes[ind];
                coordinate["position"]["x"] = targets[ind][0];
                coordinate["position"]["y"] = targets[ind][1];
                coordinate["position"]["z"] = targets[ind][2];
                networkCoordinates.push_back(coordinate);
                ind++;
            	
	            if (ind  == 10) {
                    free(corners);
                    free(code);
                    free(resultMatrix);
	                break;
	            }
            }
            free(corners);
            free(code);
            free(resultMatrix);
        }

        // sending JSON String to all Clients
        std::string jsonString = networkCoordinates.dump();
        POS_MESSAGE = jsonString;
        server.send_to_clients(POS_MESSAGE);

        for(int i = 0; i < ind ; i++)
            cout << "target " << i << ": " << targets[i][0] << ";" << targets[i][1] << ";" << targets[i][2] << " / " << codes[i] << "\n";
        imshow(contoursWindow, imgFiltered);
        isFirstStripe = true;

        isFirstMarker = true;

        if (waitKey(10) == 27) {
            break;
        }
    }
    my_thread.join();
    return 0;
}
