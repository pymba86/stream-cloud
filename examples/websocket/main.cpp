
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

// Sends a WebSocket message and prints the response
int main(int argc, char** argv)
{
    try
    {

        auto const host = "127.0.0.1";
        auto const port = "8081";
        auto const text = "client";

        // The io_context is required for all I/O
        boost::asio::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(ws.next_layer(), results.begin(), results.end());

        // Perform the websocket handshake
        ws.handshake(host, "/");


        std::string nazv_sh;
        uint32_t count = 0;

        while(true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // Send the message
                ws.write(boost::asio::buffer(std::string(text)));
                count = count + 1;
                // This buffer will hold the incoming message
                boost::beast::multi_buffer buffer;

                // Read a message into our buffer
                ws.read(buffer);
                std::cout << boost::beast::buffers(buffer.data()) << "cout: " << count << std::endl;
            }


        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);

        // If we get here then the connection is closed gracefully

        // The buffers() function helps print a ConstBufferSequence

    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
