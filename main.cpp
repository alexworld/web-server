#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>
#include <set>

using namespace boost::asio;

struct Message {
    std::string from, to, cont;
    bool read;

    Message(std::string _from, std::string _to, std::string _cont, bool _read): from(_from), to(_to), cont(_cont), read(_read) {}
};


class Observer {
public:
    virtual void react(std::string s) = 0;
};


class Model {
private:
    Model() {}
    ~Model() {}
    Model(const Model &);
    Model &operator =(const Model &);

    std::map <std::string, std::vector <Message> > user_messages;
    std::map <std::string, std::set <Observer *> > observers;
    std::map <std::string, std::string> password;

    void notify(std::string user, std::string s) {
        for (auto observer: observers[user]) {
            observer->react(s);
        }
    }

public:
    void add_observer(std::string user, Observer *observer) {
        observers[user].insert(observer);
    }

    void del_observer(std::string user, Observer *observer) {
        observers[user].erase(observer);
    }

    void login(std::string user, std::string pass) {
        if (!password.count(user) || password[user] != pass) {
            notify(user, "Login failed");
        } else {
            notify(user, "Login successful");
        }
    }

    void add_user(std::string user, std::string pass) {
        if (password.count(user)) {
            notify(user, "This username is taken");
        } else {
            user_messages[user] = std::vector <Message>();
            password[user] = pass;
            notify(user, "Register and login successful");
        }
    }

    void send_message(std::string from, std::string to, std::string cont) {
        if (!password.count(to)) {
            notify(from, "No such user");
        } else if (from == to) {
            notify(from, "You cannot sent message to yourself");
        } else {
            Message now(from, to, cont, false);
            user_messages[from].push_back(now);
            user_messages[to].push_back(now);
            notify(from, "Message sent");
            notify(to, "Message from " + from + ": " + cont);
        }
    }

    void get_messages(std::string one, std::string two) {
        if (!password.count(two)) {
            notify(one, "No such user");
        } else {
            std::string res;

            for (auto message: user_messages[one]) {
                if (message.from == two) {
                    res += two + ": " + message.cont + "\n";
                } else if (message.to == two) {
                    res += one + ": " + message.cont + "\n";
                }
            }
            notify(one, res);
        }
    }

    static Model &instance() {
        static Model model;
        return model;
    }
};
#define model Model::instance()


class Worker: public Observer {
private:
    std::shared_ptr <ip::tcp::socket> socket;
    std::shared_ptr <streambuf> buffer;
    std::string message;
    std::string terminator;

    std::string option;
    std::string user;
    std::string password;

    int action;

    void on_write(const boost::system::error_code &error, size_t) {
        if (error) {
            model.del_observer(user, this);
            boost::system::error_code now;
            socket->shutdown(ip::tcp::socket::shutdown_both, now);
            socket->close(now);
        }

        get_message();
    }

    void on_read(const boost::system::error_code &error, size_t) {
        if (error) {
            model.del_observer(user, this);
            boost::system::error_code now;
            socket->shutdown(ip::tcp::socket::shutdown_both, now);
            socket->close(now);
            return;
        }
        std::istream is(buffer.get());

        if (action == 3) {
            std::string opt;
            is >> opt;

            if (opt == "S") {
                std::string to, cont;
                is >> to;
                getline(is, cont);
                model.send_message(user, to, cont);
            } else {
                std::string to;
                is >> to;
                model.get_messages(user, to);
            }
            return;
        }

        getline(is, message);

        if (action == 0) {
            option = message;
            action++;
            send_message();
        } else if (action == 1) {
            user = message;
            action++;
            send_message();
        } else if (action == 2) {
            password = message;
            action++;

            model.add_observer(user, this);

            if (option == "R") {
                model.add_user(user, password);
            } else {
                model.login(user, password);
            }
        }
    }

    void get_message() {
        async_read_until(*socket, *buffer, terminator, boost::bind(&Worker::on_read, this, _1, _2));
    }

    void send_message(std::string s = "") {
        if (action == 0) {
            s += "Do you want to login or register? (R to register, login otherwise): ";
        } else if (action == 1) {
            s += "Login: ";
        } else if (action == 2) {
            s += "Password: ";
        } else {
            s += message;
        }
        async_write(*socket, boost::asio::buffer(s), boost::bind(&Worker::on_write, this, _1, _2));
    }

    void auth(std::string prefix = "") {
        action = 0;
        send_message(prefix);
    }

public:
    virtual void react(std::string s) {
        if (s == "Login failed" || s == "This username is taken") {
            s += "\n";
            auth(s);
        } else {
            message = s;
            message += "\n>>> ";
            send_message();
        }
    }

    Worker(std::shared_ptr <ip::tcp::socket> _socket): socket(_socket), buffer(new streambuf), message(), terminator("\n"), action(0) {
        auth();
    }
};


class Factory {
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
    void add_query(std::shared_ptr <ip::tcp::socket> socket) {
        workers.push_back(std::shared_ptr <Worker>(new Worker(socket)));
    }

    static Factory &instance() {
        static Factory factory;
        return factory;
    }
};


class Acceptor {
private:
    unsigned short port;
    static io_service service;
    ip::tcp::acceptor acceptor;

    void handle_accept(std::shared_ptr <ip::tcp::socket> socket, const boost::system::error_code &error) {
        if (error) {
            return;
        }
        Factory::instance().add_query(socket);
        std::shared_ptr <ip::tcp::socket> newsocket(new ip::tcp::socket(service));
        start_accept(newsocket);
    }

    void start_accept(std::shared_ptr <ip::tcp::socket> socket) {
        acceptor.async_accept(*socket, boost::bind(&Acceptor::handle_accept, this, socket, _1));
    }

    Acceptor &operator =(const Acceptor &);
    Acceptor(const Acceptor &);

public:
    Acceptor(unsigned short _port): port(_port),
    acceptor(ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), port))) {
        std::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));
        start_accept(socket);
    }

    static void start_working() {
        service.run();
    }
};
io_service Acceptor::service;


int main() {
    Acceptor server1(5000);
    Acceptor server2(4000);
    Acceptor::start_working();
}
