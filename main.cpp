#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

using namespace boost::asio;

class Worker
{
private:
    //TODO
    // HTTP work
    // block copy
    // shared ptrs????

    std::shared_ptr <ip::tcp::socket> socket;
    std::shared_ptr <streambuf> buffer;
    std::string message;
    std::string terminator;

    void on_write(const boost::system::error_code &, size_t)
    {
/*        boost::system::error_code now;
        socket->shutdown(ip::tcp::socket::shutdown_both, now);
        socket->close(now);
        size = size;*/

        return;
    }

    void on_read(const boost::system::error_code &error, size_t)
    {
        std::istream is(buffer.get());
        getline(is, message);
        message += terminator;

        std::string result = "Got this message: " + message;
        std::cout << message;
        std::cout.flush();

        async_write(*socket, boost::asio::buffer(result), boost::bind(&Worker::on_write, this, _1, _2));
    
        if (error) {
            boost::system::error_code now;
            socket->shutdown(ip::tcp::socket::shutdown_both, now);
            socket->close(now);
            std::cout << "connection terminated" << std::endl;
            return;
        }

        async_read_until(*socket, *buffer, terminator, boost::bind(&Worker::on_read, this, _1, _2));
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
    Factory() {}
    ~Factory() {}
    Factory(const Factory &);
    Factory &operator =(const Factory &);

    // TODO
    // boost::thread
    // queue of sockets
    std::vector <std::shared_ptr <Worker> > workers;

public:
    void add_query(std::shared_ptr <ip::tcp::socket> socket)
    {
        workers.push_back(std::shared_ptr <Worker>(new Worker(socket)));
    }

    static Factory &instance()
    {
        static Factory factory;
        return factory;
    }
};


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
        Factory::instance().add_query(socket);
        std::shared_ptr <ip::tcp::socket> newsocket(new ip::tcp::socket(service));
        start_accept(newsocket);
    }

    void start_accept(std::shared_ptr <ip::tcp::socket> socket)
    {
        acceptor.async_accept(*socket, boost::bind(&Acceptor::handle_accept, this, socket, _1));
    }

    Acceptor &operator =(const Acceptor &);
    Acceptor(const Acceptor &);

public:
    Acceptor(unsigned short _port): port(_port),
    acceptor(ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port)))
    {
        std::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));
        start_accept(socket);
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
