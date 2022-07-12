#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "MarkerTracker.h"
#include "AtoB.h"


int main()
{
	cv::Mat img_bgr;
	cv::VideoCapture cap;
	cap.open(0);
	std::unique_ptr<MarkerTracker> markerTracker = std::make_unique<MarkerTracker>(0.04346, 50, 50);
	float rotB[] = { 0.f, 0.f, 0.f };
	float transB[] = { 0.f, 0.f, 0.f };

	while (cap.read(img_bgr))
	{
		float resultMatrix[16];
		markerTracker->findMarker(img_bgr, resultMatrix);
		float* coordinates = AtoB(resultMatrix, rotB, transB);

		free(coordinates);
		if (cv::waitKey(10) == 27)
		{
			break;
		}
	}

	std::cout << "Terminating!" << std::endl;
	return 0;
}
