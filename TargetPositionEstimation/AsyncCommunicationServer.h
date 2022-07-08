#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <string>

using boost::asio::ip::tcp;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
	typedef boost::shared_ptr<tcp_connection> pointer;

	static pointer create(boost::asio::io_context& io_context);
	
	// getter for initial socket. Used in the acceptor loop
	tcp::socket& socket();

	// sends message to socket. ABSOLUTLY not thread safe, pls dont change message
	void sendMessage(std::string& message);

	bool closeMe_ = false;

private:
	tcp_connection(boost::asio::io_context& io_context)
		: socket_(io_context)
	{
	}

	// function to be called after async_write. Sets closeMe if encountering an error
	void handle_write(const boost::system::error_code& err, size_t /*bytes_transferred*/);
	tcp::socket socket_;
};

class tcp_server
{
public:
	tcp_server(boost::asio::io_context& io_context)
		: io_context_(io_context),
		acceptor_(io_context, tcp::endpoint(tcp::v4(), 20042))
	{
		start_accept();
	}

	// Triggering send message for all clients. 100% not thread safe, pls don't change message.
	void send_to_clients(std::string& message);

	// vector storing all client connections
	std::vector<tcp_connection::pointer> clients_ = {};

private:
	// Accept loop doing a asynchronous accept and calling handle_accept on new connection
	void start_accept();

	// function to be called on successful accept, adds connection to the client vector and triggers message send to all clients
	void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error);

	// io context the server runs in and the acceptor for the server socket
	boost::asio::io_context& io_context_;
	tcp::acceptor acceptor_;
};
