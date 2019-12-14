#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sstream>
#include <string>

using namespace std;
using namespace boost::asio;

struct ENV{
	string req_method;
	string req_uri;
	string q_str;
	string server_protocol;
	string http_host;
	string server_addr;
	string server_port;
	string remote_addr;
	string remote_port;
};

io_service global_io_service;

class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
  enum { max_length = 1024 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;

 public:
  EchoSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { do_read(); }

 private:
  void do_read() {
    auto self(shared_from_this());
    _socket.async_read_some(
        buffer(_data, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
         //if (!ec) do_write(length);
		if(!ec){
			string str;
			str.append(_data.data(),_data.data()+length);
			struct ENV env;
			parse_request(str,env);
			if(!strcmp(env.req_uri.c_str(),"/panel.cgi")){
				cout << "panel.cgi" << endl;
				pid = fork();
				if(pid == 0){
					dup2(_socket,STDOUT);
					dup2(_socket,STDIN);
					execl(env.req_uri.c_str(),env.req_uri.c_str(),NULL);
				}else{
					_socket.close();	
				}
			}
			do_read();
		}	
        });
  }

  void do_write(size_t length) {
    auto self(shared_from_this());
    _socket.async_send(
        buffer(_data, length),
        [this, self](boost::system::error_code ec, size_t /* length */) {
          if (!ec) do_read();
        });
  }

  void parse_request(string request,struct ENV& env){
  	vector<string> parsed_request;
	stringstream ss1(request);
	string str1;
	for(int line=0 ; line<2&&getline(ss1,str1) ; line++){
		stringstream ss2(str1);
		string str2;
		getline(ss2,str2,'\r');
		stringstream ss3(str2);
		string str3;
		if(line==0){
			ss3 >> env.req_method;
			ss3 >> env.req_uri;
			ss3 >> env.server_protocol;			
		}else if(line==1){
			ss3 >> str3;
			ss3 >> env.http_host;
		}
	}	
  }
};

class EchoServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;

 public:
  EchoServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
    do_accept();
  }

 private:
  void do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
	if (!ec) make_shared<EchoSession>(move(_socket))->start();	
      	do_accept();
    });
  }
};

int main(int argc, char* const argv[]) {
  if (argc != 2) {
    cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }

  try {
    unsigned short port = atoi(argv[1]);
    EchoServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
