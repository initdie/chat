#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <deque>
#include <string>

using boost::asio::ip::tcp;

// Message structure to hold chat data
struct ChatMessage {
    static constexpr size_t max_length = 1024;  // Maximum message size (1KB)
    char data[max_length];                      // Raw message data buffer
    size_t length;                              // Actual length of message
};

// Handles each client connection separately
class ChatSession : public std::enable_shared_from_this<ChatSession> {
    // enable_shared_from_this allows safe creation of shared_ptr to 'this'
public:
    // Constructor takes ownership of socket
    ChatSession(tcp::socket socket)
        : socket_(std::move(socket)) {
        write_msg_.length = 0;  // Initialize empty write buffer
    }

    // Start the session
    void start() {
        do_read();   // Start reading from client
        do_write();  // Start writing to client
    }

private:
    // Asynchronous read operation
    void do_read() {
        auto self(shared_from_this());  // Keep object alive during async operation
        socket_.async_read_some(        // Start non-blocking read
            boost::asio::buffer(read_msg_.data, ChatMessage::max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {  // If no error
                    read_msg_.length = length;
                    // Display received message
                    std::cout << "Received: " << std::string(read_msg_.data, length) << std::endl;
                    
                    // Copy to write buffer for echo
                    std::memcpy(write_msg_.data, read_msg_.data, length);
                    write_msg_.length = length;
                    
                    do_read();  // Continue reading
                }
            });
    }

    // Asynchronous write operation
    void do_write() {
        auto self(shared_from_this());  // Keep object alive
        if (write_msg_.length > 0) {    // If we have data to write
            boost::asio::async_write(   // Start non-blocking write
                socket_,
                boost::asio::buffer(write_msg_.data, write_msg_.length),
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        write_msg_.length = 0;  // Clear write buffer
                        do_write();            // Continue write loop
                    }
                });
        } else {
            // No data to write - wait and check again
            auto timer = std::make_shared<boost::asio::steady_timer>(
                socket_.get_executor(), 
                std::chrono::milliseconds(100)  // 100ms delay
            );
            timer->async_wait([this, self, timer](boost::system::error_code) {
                do_write();  // Check again after delay
            });
        }
    }

    tcp::socket socket_;        // Socket for client connection
    ChatMessage read_msg_;      // Buffer for incoming data
    ChatMessage write_msg_;     // Buffer for outgoing data
};

// Main server class
class ChatServer {
public:
    // Constructor initializes server on specified port
    ChatServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        std::cout << "Chat server v1.0 started" << std::endl;
        do_accept();  // Start accepting connections
    }

private:
    // Accept new client connections
    void do_accept() {
        acceptor_.async_accept(  // Start non-blocking accept
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Create new session for client
                    std::make_shared<ChatSession>(std::move(socket))->start();
                }
                do_accept();  // Continue accepting
            });
    }

    tcp::acceptor acceptor_;  // Accepts incoming connections
};
