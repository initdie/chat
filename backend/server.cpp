#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <deque>
#include <string>

using boost::asio::ip::tcp;

// Message structure for chat messages
struct ChatMessage {
    static constexpr size_t max_length = 1024;  // Maximum message size
    char data[max_length];                      // Buffer for message data
    size_t length;                             // Actual message length
};

// Handles individual client connections
class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket)
        : socket_(std::move(socket)) {
        write_msg_.length = 0;  // Initialize write buffer
    }

    void start() {
        do_read();
        do_write();  // Start both reading and writing loops
    }

private:
    // Asynchronously read from client
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(read_msg_.data, ChatMessage::max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    read_msg_.length = length;
                    std::cout << "Received: " << std::string(read_msg_.data, length) << std::endl;
                    
                    // Copy message to write buffer for echo
                    std::memcpy(write_msg_.data, read_msg_.data, length);
                    write_msg_.length = length;
                    
                    do_read();  // Continue reading
                }
            });
    }

    // Asynchronously write back to client
    void do_write() {
        auto self(shared_from_this());
        if (write_msg_.length > 0) {  // Only write if we have data
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_msg_.data, write_msg_.length),
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        write_msg_.length = 0;  // Reset write buffer
                        do_write();  // Continue writing loop
                    }
                });
        } else {
            // No data to write, wait a bit and check again
            auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor(), 
                std::chrono::milliseconds(100));
            timer->async_wait([this, self, timer](boost::system::error_code) {
                do_write();
            });
        }
    }

    tcp::socket socket_;        // Socket for client connection
    ChatMessage read_msg_;      // Buffer for incoming messages
    ChatMessage write_msg_;     // Buffer for outgoing messages
};

// Main server class
class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        std::cout << "Chat server v1.0 started" << std::endl;
        do_accept();  // Start accepting connections
    }

private:
    // Asynchronously accept new connections
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Create new session for each client
                    std::make_shared<ChatSession>(std::move(socket))->start();
                }
                do_accept();  // Continue accepting new connections
            });
    }

    tcp::acceptor acceptor_;  // Accepts incoming connections
};
