#include <iostream>
#include <boost/asio.hpp>
#include "server.cpp"

int main() {
    try {
        boost::asio::io_context io_context;
        ChatServer server(io_context, 8080);
        std::cout << "Chat server started on port 8080" << std::endl;
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
