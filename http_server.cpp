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

class HttpSession : public enable_shared_from_this<HttpSession> {
 private:
  enum { max_length = 1024 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;

 public:
  HttpSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { do_read(); }

 private:
  void do_read() {
    auto self(shared_from_this());
    _socket.async_read_some(
        buffer(_data, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
		if(!ec){
			string req = _data.data();
			
			struct ENV env;
			parse_request(req,env);
			env.server_addr = _socket.local_endpoint().address().to_string();
			env.server_port = to_string(_socket.local_endpoint().port());
			env.remote_addr = _socket.remote_endpoint().address().to_string();
			env.remote_port = to_string(_socket.remote_endpoint().port());			

			global_io_service.notify_fork(io_service::fork_prepare);
			pid_t pid = fork();
			if(pid == 0){	
				global_io_service.notify_fork(io_service::fork_child);
				set_env(env);
				
				dup2(_socket.native_handle(),STDIN_FILENO);
				dup2(_socket.native_handle(),STDOUT_FILENO);
				dup2(_socket.native_handle(),STDERR_FILENO);
					
				cout << env.server_protocol + "200 OK" << endl;
					
				if(execl(env.req_uri.c_str(),env.req_uri.c_str(),NULL)<0){
					cout << env.server_protocol + "500 Internal Server Error" << endl;
					_socket.close();
				}

			}else if(pid > 0){
				global_io_service.notify_fork(io_service::fork_parent);
				_socket.close();	
			}
		}	
        });
  }

  void parse_request(string request,struct ENV& env){
	stringstream ss1(request);
	string str;
	for(int line=0 ; line<2&&getline(ss1,str) ; line++){
		stringstream ss2(str);
		getline(ss2,str,'\r');
		stringstream ss3(str);
		if(line==0){
			ss3 >> env.req_method;
			ss3 >> str;
			ss3 >> env.server_protocol;	
			stringstream ss4(str);
			getline(ss4,str,'?');
			env.req_uri = str.substr(1,str.size()-1);
			getline(ss4,env.q_str,'?');		
		}else if(line==1){
			ss3 >> str;
			ss3 >> env.http_host;
		}
	}	
  }

  void set_env(struct ENV& env){
  	clearenv();
	setenv("REQUEST_METHOD",env.req_method.c_str(),1);
	setenv("REQUEST_URI",env.req_uri.c_str(),1);
	setenv("QUERY_STRING",env.q_str.c_str(),1);
	setenv("SERVER_PROTOCOL",env.server_protocol.c_str(),1);
	setenv("HTTP_HOST",env.http_host.c_str(),1);
	setenv("SERVER_ADDR",env.server_addr.c_str(),1);
	setenv("SEVRER_PORT",env.server_port.c_str(),1);
	setenv("REMOTE_ADDR",env.remote_addr.c_str(),1);
	setenv("REMOTE_PORT",env.remote_port.c_str(),1);
  }

};

class HttpServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;

 public:
  HttpServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
    do_accept();
  }

 private:
  void do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
	if (!ec) make_shared<HttpSession>(move(_socket))->start();	
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
    HttpServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
