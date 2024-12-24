#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <deque>
#include <string>

using boost::asio::ip::tcp;

// Message structure to handle chat messages
struct ChatMessage {
    static constexpr size_t max_length = 1024;
    char data[max_length];
    size_t length;
};

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(read_msg_.data, ChatMessage::max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    read_msg_.length = length;
                    std::cout << "Received: " << std::string(read_msg_.data, length) << std::endl;
                    do_write();
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(read_msg_.data, read_msg_.length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            });
    }

    tcp::socket socket_;
    ChatMessage read_msg_;
};

class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<ChatSession>(std::move(socket))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};
