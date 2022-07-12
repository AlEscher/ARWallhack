#include <ctime>
#include <vector>
#include <iostream>
#include <string>
#include <thread>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <chrono>

#include "AsyncCommunicationServer.h"

using boost::asio::ip::tcp;

std::string GLOBAL_MESSAGE = "test";


tcp_connection::pointer tcp_connection::create(boost::asio::io_context& io_context)
{
	return pointer(new tcp_connection(io_context));
}

// getter for initial socket. Used in the acceptor loop
tcp::socket& tcp_connection::socket()
{
	return socket_;
}

// sends message to socket. ABSOLUTLY not thread safe, pls dont change message
void tcp_connection::sendMessage(std::string& message)
{
	boost::asio::async_write(socket_, boost::asio::buffer(message), boost::bind(&tcp_connection::handle_write, shared_from_this(), boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

// function to be called after async_write. Sets closeMe if encountering an error
void tcp_connection::handle_write(const boost::system::error_code& err, size_t)
{
	if (err) {
		socket_.close();
		closeMe_ = true;
	}
}

// Triggering send message for all clients. 100% not thread safe, pls don't change message.
void tcp_server::send_to_clients(std::string& message) {

	// loops through all clients for sending
	for (int i = 0; i < clients_.size(); i++) {
		clients_.at(i)->sendMessage(message);
	}
	// deletes clients that are closed and ran into errors
	for (int i = clients_.size() - 1; i >= 0; i--) {
		if (clients_.at(i)->closeMe_) {
			clients_.erase(clients_.begin() + i);
		}
	}

}

// Accept loop doing a asynchronous accept and calling handle_accept on new connection
void tcp_server::start_accept()
{
	tcp_connection::pointer new_connection = tcp_connection::create(io_context_);

	acceptor_.async_accept(new_connection->socket(), boost::bind(&tcp_server::handle_accept, this, new_connection, boost::asio::placeholders::error));

}


// function to be called on successful accept, adds connection to the client vector and triggers message send to all clients
void tcp_server::handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error)
{
	if (!error)
	{
		clients_.push_back(new_connection);
		std::cout << "Connection from: " << new_connection->socket().remote_endpoint().address().to_string() << ":" << std::to_string(new_connection->socket().remote_endpoint().port()) << std::endl;
	}

	start_accept();
}
