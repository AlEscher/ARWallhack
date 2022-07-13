#include <iostream>
#include <algorithm>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "MarkerTracker.h"
#include "AtoB.h"
#include "AsyncCommunicationServer.h"
#include "json.hpp"


// JSON String that is sent to all Clients
std::string POS_MESSAGE;
const std::array<int, 6> validMarkerIDs {0x0690, 0x0272, 0x0B44, 0x1228, 0x1C44, 0x005A};

int main()
{
	// Setup server acceptor loop in own thread
	boost::asio::io_context io_context;
	tcp_server server(io_context);
	std::thread my_thread([&]() { io_context.run(); });

	// Setup OpenCV
	cv::Mat img_bgr;
	cv::VideoCapture cap;
	cap.open(0);
	std::unique_ptr<MarkerTracker> markerTracker = std::make_unique<MarkerTracker>(0.04346, 50, 50);
	// The rotation and translation component of the fixed 'B' transformation (see scene graph)
	float rotB[] = { 0.f, 0.f, 0.f };
	// -x = links, x = rechts | -y = unten, y = oben | -z = auﬂen, z = innen
	float transB[] = { -0.06f, -0.21f, -0.05f };

	while (cap.read(img_bgr))
	{
		// Mapping from Marker ID to pose estimation result matrix
		std::unordered_map<int, std::array<float, 16>> trackedMarkers;
		markerTracker->findMarker(img_bgr, trackedMarkers);
		// JSON object to parse
		auto networkCoordinates = nlohmann::json::array();

		// Iterate through all markers that were found in the image and add them to the JSON array
		for (auto it = trackedMarkers.begin(); it != trackedMarkers.end(); ++it) 
		{
			if (std::count(validMarkerIDs.begin(), validMarkerIDs.end(), it->first) < 1)
			{
				continue;
			}
			// Convert the tracked position into the Marker's coordinate system
			float* coordinates = AtoB(it->second.data(), rotB, transB);
		
			// Construct JSON object and add it to the array
			nlohmann::json coordinate;
			coordinate["id"] = it->first;
			coordinate["position"]["x"] = coordinates[0];
			coordinate["position"]["y"] = coordinates[1];
			coordinate["position"]["z"] = coordinates[2]*-1.f;
			networkCoordinates.push_back(coordinate);
			free(coordinates);
		}

		// Parse JSON into string and send it to all clients with an active connection
		std::string jsonString = networkCoordinates.dump() + "\n";
		POS_MESSAGE = jsonString;
		server.send_to_clients(POS_MESSAGE);
		
		if (cv::waitKey(10) == 27)
		{
			break;
		}
	}

	std::cout << "Terminating!" << std::endl;
	return 0;
}
