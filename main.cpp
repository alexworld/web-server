#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

using namespace boost::asio;

class Worker
{
private:
    std::shared_ptr <ip::tcp::socket> socket;
    std::shared_ptr <streambuf> buffer;
    std::string message;
    std::string terminator;

    void on_write(const boost::system::error_code &error, size_t size)
    {
        boost::system::error_code now;
        socket->shutdown(ip::tcp::socket::shutdown_both, now);
        socket->close(now);
        size = size;

        if (error) {
            return;
        }
        return;
    }

    void on_read(const boost::system::error_code &, size_t)
    {
        std::istream is(buffer.get());
        is >> std::noskipws;
        char ch;

        while (is >> ch) {
            message += ch;
        }
        std::string result = "Got this message: " + message;
        std::cout << message;
        std::cout.flush();

        async_write(*socket, boost::asio::buffer(result), boost::bind(&Worker::on_write, this, _1, _2));
    /*
        if (error) {
            boost::system::error_code now;
            c.socket->shutdown(ip::tcp::socket::shutdown_both, now);
            c.socket->close(now);
            std::cout << "connection terminated" << std::endl;
            return;
        }

        async_read_until(*c.socket, *c.buffer, terminator, boost::bind(on_read, c, _1, _2));*/
    }

public:
    Worker(std::shared_ptr <ip::tcp::socket> _socket): socket(_socket), buffer(new streambuf), message(), terminator("\n")
    {
        async_read_until(*socket, *buffer, terminator, boost::bind(&Worker::on_read, this, _1, _2));
    }
};

class Factory
{
private:
    // boost::thread
    // singleton
    // queue of sockets
    std::vector <std::shared_ptr <Worker> > workers;

public:
    void add_query(std::shared_ptr <ip::tcp::socket> socket)
    {
        //workers.push_back(new Worker(socket));
        workers.push_back(std::shared_ptr <Worker>(new Worker(socket)));
    }
        
} factory;

class Acceptor
{
private:
    unsigned short port;
    static io_service service;
    ip::tcp::acceptor acceptor;

    void handle_accept(std::shared_ptr <ip::tcp::socket> socket, const boost::system::error_code &error)
    {
        if (error) {
            return;
        }
        factory.add_query(socket);
        std::shared_ptr <ip::tcp::socket> newsocket(new ip::tcp::socket(service));
        start_accept(newsocket);
    }

    void start_accept(std::shared_ptr <ip::tcp::socket> socket)
    {
        acceptor.async_accept(*socket, boost::bind(&Acceptor::handle_accept, this, socket, _1));
    }

public:
    Acceptor(unsigned short _port): port(_port),
    acceptor(ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)))
    {
        std::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));
        start_accept(socket);
//        service.run();
    }

    static void start_working()
    {
        service.run();
    }
};

io_service Acceptor::service;

int main()
{
    Acceptor server1(5000);
    Acceptor server2(4000);
    Acceptor::start_working();
}