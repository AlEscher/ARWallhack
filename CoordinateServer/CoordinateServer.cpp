#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "MarkerTracker.h"
#include "AtoB.h"
#include "AsyncCommunicationServer.h"
#include "json.hpp"


// JSON String that is sent to all Clients
std::string POS_MESSAGE;

int main()
{
	// setup server acceptor loop in own thread
	boost::asio::io_context io_context;
	tcp_server server(io_context);
	std::thread my_thread([&]() { io_context.run(); });

	// setup openCV marker tracker
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

		// JSON object to parse
		auto networkCoordinates = nlohmann::json::array();

		// sending JSON String to all Clients
		std::string jsonString = networkCoordinates.dump();
		POS_MESSAGE = jsonString;
		server.send_to_clients(POS_MESSAGE);



		free(coordinates);
		if (cv::waitKey(10) == 27)
		{
			break;
		}
	}

	std::cout << "Terminating!" << std::endl;
	return 0;
}
