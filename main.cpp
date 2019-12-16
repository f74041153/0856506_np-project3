#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sstream>
#include <string>
#include <fstream>

#define MAXHOST 5

using namespace std;
using namespace boost::asio;

struct ENV
{
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

struct RemoteServer
{
	string host;
	string port;
	string file;
	int id;
};

io_service global_io_service;

void set_remote_server(struct RemoteServer rs[],struct ENV env)
{
	string q_str = env.q_str;
	stringstream ss1(q_str);
	string str;
	for (int i = 0; i < MAXHOST; i++)
	{
		getline(ss1, str, '&');
		rs[i].host = str.substr(3, str.size() - 1);
		getline(ss1, str, '&');
		rs[i].port = str.substr(3, str.size() - 1);
		getline(ss1, str, '&');
		rs[i].file = str.substr(3, str.size() - 1);
		rs[i].id = i;
	}
}

bool rs_selected(struct RemoteServer rs)
{
	if (rs.host.size() && rs.port.size() && rs.file.size())
		return true;
	return false;
}

class ConsoleSession;

class HttpSession : public enable_shared_from_this<HttpSession>
{
	friend class ConsoleSession;

private:
	enum
	{
		max_length = 1024
	};
	ip::tcp::socket _socket;
	array<char, max_length> _data;
	struct ENV env;
	struct RemoteServer RS[MAXHOST];

public:
	//ip::tcp::socket _socket;
	HttpSession(ip::tcp::socket socket) : _socket(move(socket)) {}
	void start() { do_read(); }

private:
	void do_read()
	{
		auto self(shared_from_this());
		_socket.async_read_some(
			buffer(_data, max_length),
			[this, self](boost::system::error_code ec, size_t length) {
				if (!ec)
				{
					string req(_data.data(), length);

					parse_request(req, env);
					env.server_addr = _socket.local_endpoint().address().to_string();
					env.server_port = to_string(_socket.local_endpoint().port());
					env.remote_addr = _socket.remote_endpoint().address().to_string();
					env.remote_port = to_string(_socket.remote_endpoint().port());
					//set_env(env);

					response();
				}
			});
	}
	void response()
	{
		auto self(shared_from_this());
		string ok = env.server_protocol + "200 OK\n";
		_socket.async_send(
			buffer(ok),
			[this, self](boost::system::error_code ec, size_t /* length */) {
				if (!ec)
				{
					if (!strcmp(env.req_uri.c_str(), "panel.cgi"))
					{
						panel_cgi();
					}
					else if (!strcmp(env.req_uri.c_str(), "console.cgi"))
					{
						set_remote_server(RS,env);
						console_cgi(RS);
					}
				}
			});
	}

	void parse_request(string request, struct ENV &env)
	{
		stringstream ss1(request);
		string str;
		for (int line = 0; line < 2 && getline(ss1, str); line++)
		{
			stringstream ss2(str);
			getline(ss2, str, '\r');
			stringstream ss3(str);
			if (line == 0)
			{
				ss3 >> env.req_method;
				ss3 >> str;
				ss3 >> env.server_protocol;
				stringstream ss4(str);
				getline(ss4, str, '?');
				env.req_uri = str.substr(1, str.size() - 1);
				getline(ss4, env.q_str, '?');
			}
			else if (line == 1)
			{
				ss3 >> str;
				ss3 >> env.http_host;
			}
		}
	}

	void panel_cgi()
	{
		auto self(shared_from_this());
		string panel = "Content-type: text/html\r\n\r\n"
					   "<!DOCTYPE html>\n"
					   "<html lang=\"en\">\n"
					   "  <head>\n"
					   "    <title>NP Project 3 Panel</title>\n"
					   "    <link\n"
					   "      rel=\"stylesheet\"\n"
					   "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
					   "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
					   "      crossorigin=\"anonymous\"\n"
					   "    />\n"
					   "    <link\n"
					   "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
					   "      rel=\"stylesheet\"\n"
					   "    />\n"
					   "    <link\n"
					   "      rel=\"icon\"\n"
					   "      type=\"image/png\"\n"
					   "      href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\n"
					   "    />\n"
					   "    <style>\n"
					   "      * {\n"
					   "        font-family: 'Source Code Pro', monospace;\n"
					   "      }\n"
					   "    </style>\n"
					   "  </head>\n"
					   "  <body class=\"bg-secondary pt-5\">\n"
					   "<form action=\"console.cgi\" method=\"GET\">\n"
					   "      <table class=\"table mx-auto bg-light\" style=\"width: inherit\">\n"
					   "        <thead class=\"thead-dark\">\n"
					   "          <tr>\n"
					   "            <th scope=\"col\">#</th>\n"
					   "            <th scope=\"col\">Host</th>\n"
					   "            <th scope=\"col\">Port</th>\n"
					   "            <th scope=\"col\">Input File</th>\n"
					   "          </tr>\n"
					   "        </thead>\n"
					   "        <tbody>";
		string test_case_menu = "<option value=\"t1.txt\">t1.txt</option><option value=\"t2.txt\">t2.txt</option><option value=\"t3.txt\">t3.txt</option><option value=\"t4.txt\">t4.txt</option><option     value=\"t5.txt\">t5.txt</option><option value=\"t6.txt\">t6.txt</option><option value=\"t7.txt\">t7.txt</option><option value=\"t8.txt\">t8.txt</option><option value=\"t9.txt\">t9.txt</option><option value=\"t10.txt\">t10.txt</option>";
		for (int i = 0; i < MAXHOST; i++)
		{
			panel += "<tr>\n"
					 "            <th scope=\"row\" class=\"align-middle\">Session " +
					 to_string(i + 1) + "</th>\n"
										"            <td>\n"
										"              <div class=\"input-group\">\n"
										"                <select name=\"h" +
					 to_string(i) + "\" class=\"custom-select\">\n"
									"                  <option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"nplinux6.cs.nctu.edu.tw\">nplinux6</option><option value=\"nplinux7.cs.nctu.edu.tw\">nplinux7</option><option value=\"nplinux8.cs.nctu.edu.tw\">nplinux8</option><option value=\"nplinux9.cs.nctu.edu.tw\">nplinux9</option><option value=\"nplinux10.cs.nctu.edu.tw\">nplinux10</option><option value=\"nplinux11.cs.nctu.edu.tw\">nplinux11</option><option value=\"nplinux12.cs.nctu.edu.tw\">nplinux12</option>\n"
									"                </select>\n"
									"                <div class=\"input-group-append\">\n"
									"                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
									"                </div>\n"
									"              </div>\n"
									"            </td>\n"
									"            <td>\n"
									"              <input name=\"p" +
					 to_string(i) + "\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
									"            </td>\n"
									"            <td>\n"
									"              <select name=\"f" +
					 to_string(i) + "\" class=\"custom-select\">\n"
									"                <option></option>\n"
									"                " +
					 test_case_menu + "\n"
									  "              </select>\n"
									  "            </td>\n"
									  "          </tr>";
		}

		panel += "<tr>\n"
				 "            <td colspan=\"3\"></td>\n"
				 "            <td>\n"
				 "              <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\n"
				 "            </td>\n"
				 "          </tr>\n"
				 "        </tbody>\n"
				 "      </table>\n"
				 "    </form>\n"
				 "  </body>\n"
				 "</html>";
		_socket.async_send(
			buffer(panel),
			[this, self](boost::system::error_code ec, size_t length) {
				if (!ec)
				{
					_socket.close();
				}
			});
	}

	void console_cgi(struct RemoteServer rs[]);
};

class HttpServer
{
private:
	ip::tcp::acceptor _acceptor;
	ip::tcp::socket _socket;

public:
	HttpServer(short port)
		: _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
		  _socket(global_io_service)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		_acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
			if (!ec)
				make_shared<HttpSession>(move(_socket))->start();
			do_accept();
		});
	}
};

class ConsoleSession : public enable_shared_from_this<ConsoleSession>
{
private:
	enum
	{
		max_length = 4096
	};
	ip::tcp::resolver::query q;
	ip::tcp::resolver resolv{global_io_service};
	ip::tcp::socket tcp_socket{global_io_service};
	ip::tcp::socket client_socket{global_io_service};
	array<char, max_length> _data;
	struct RemoteServer RS;
	shared_ptr<HttpSession> HS;
	ifstream testcase;
	string shell2client, cmd;

public:
	ConsoleSession(struct RemoteServer rs, HttpSession *hs) : q(rs.host, rs.port), RS(rs), HS(hs->shared_from_this()), testcase("test_case/" + rs.file) {}

	void start()
	{
		do_resolve();
	}

private:
	void do_resolve()
	{
		auto self(shared_from_this());
		resolv.async_resolve(q, [this, self](boost::system::error_code ec, ip::tcp::resolver::iterator it) {
			if (!ec)
			{
				do_connect(it);
			}
		});
	}
	void do_connect(ip::tcp::resolver::iterator it)
	{
		auto self(shared_from_this());
		tcp_socket.async_connect(*it, [this, self](boost::system::error_code ec) {
			if (!ec)
			{
				do_read();
			}
		});
	}
	void do_read()
	{
		auto self(shared_from_this());
		tcp_socket.async_read_some(
			buffer(_data, max_length),
			[this, self](boost::system::error_code ec, size_t length) {
				if (!ec)
				{
					string content(_data.data(), length);
					output2client(RS.id, content);
				}
				else
				{
					testcase.close();
				}
			});
	}

	void do_write(string to_rs)
	{
		auto self(shared_from_this());
		tcp_socket.async_send(buffer(to_rs), [this, self](boost::system::error_code ec, size_t /* length */) {
			if (!ec)
				do_read();
		});
	}

	bool percent_exist(string str)
	{
		for (unsigned int i = 0; i < str.size(); i++)
		{
			if (str[i] == '%')
				return true;
		}
		return false;
	}

	void html_escape(string &data)
	{
		string buffer;
		buffer.reserve(data.size());
		for (size_t pos = 0; pos != data.size(); ++pos)
		{
			switch (data[pos])
			{
			case '&':
				buffer.append("&amp;");
				break;
			case '\"':
				buffer.append("&quot;");
				break;
			case '\'':
				buffer.append("&apos;");
				break;
			case '<':
				buffer.append("&lt;");
				break;
			case '>':
				buffer.append("&gt;");
				break;
			case '\n':
				buffer.append("&NewLine;");
				break;
			case '\r':
				buffer.append("");
				break;
			default:
				buffer.append(&data[pos], 1);
				break;
			}
		}
		data.swap(buffer);
	}

	void output2client(int rsid, string content)
	{
		auto self(shared_from_this());
		shell2client = content;
		html_escape(content);
		string rsp = "<script>document.getElementById('s" + to_string(rsid) + "').innerHTML += \'" + content + "\';</script>";
		HS->_socket.async_send(
			buffer(rsp),
			[this, self](boost::system::error_code ec, size_t /* length */) {
				if (!ec)
				{
					if (percent_exist(shell2client))
					{
						string cmd;
						getline(testcase, cmd);
						cmd += "\n";
						output_command(RS.id, cmd);
					}
					else
					{
						do_read();
					}
				}
			});
	}

	void output_command(int rsid, string content)
	{
		// client <- console -> RS
		auto self(shared_from_this());
		cmd = content;
		html_escape(content);
		string rsp = "<script>document.getElementById('s" + to_string(rsid) + "').innerHTML += \'" + content + "\';</script>";
		HS->_socket.async_send(
			buffer(rsp),
			[this, self](boost::system::error_code ec, size_t /* length */) {
				if (!ec)
				{
					do_write(cmd);
				}
			});
	}
};

void HttpSession::console_cgi(struct RemoteServer rs[])
{
	auto self(shared_from_this());
	string console = "Content-Type: text/html\n\n"
					 "<!DOCTYPE html>\n"
					 "<html lang=\"en\">\n"
					 "  <head>\n"
					 "    <meta charset=\"UTF-8\" />\n"
					 "    <title>NP Project 3 Console</title>\n"
					 "    <link\n"
					 "      rel=\"stylesheet\"\n"
					 "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
					 "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
					 "      crossorigin=\"anonymous\"\n"
					 "    />\n"
					 "    <link\n"
					 "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
					 "      rel=\"stylesheet\"\n"
					 "    />\n"
					 "    <link\n"
					 "      rel=\"icon\"\n"
					 "      type=\"image/png\"\n"
					 "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
					 "    />\n"
					 "    <style>\n"
					 "      * {\n"
					 "        font-family: 'Source Code Pro', monospace;\n"
					 "        font-size: 1rem !important;\n"
					 "      }\n"
					 "      body {\n"
					 "        background-color: #212529;\n"
					 "      }\n"
					 "      pre {\n"
					 "        color: #cccccc;\n"
					 "      }\n"
					 "      b {\n"
					 "        color: #ffffff;\n"
					 "      }\n"
					 "    </style>\n"
					 "  </head>\n"
					 "  <body>\n"
					 "    <table class=\"table table-dark table-bordered\">\n"
					 "      <thead>\n"
					 "        <tr>\n";

	for (int i = 0; i < MAXHOST; i++)
	{
		if (rs_selected(rs[i]))
		{
			console += "          <th scope=\"col\">" + rs[i].host + ":" + rs[i].port + "</th>\n";
		}
	}

	console += "        </tr>\n"
			   "      </thead>\n"
			   "      <tbody>\n"
			   "        <tr>\n";

	for (int i = 0; i < MAXHOST; i++)
	{
		if (rs_selected(rs[i]))
		{
			console += "          <td><pre id=\"s" + to_string(i) + "\" class=\"mb-0\"></pre></td>\n";
		}
	}

	console += "        </tr>\n"
			   "      </tbody>\n"
			   "    </table>\n"
			   "  </body>\n"
			   "</html>";
	_socket.async_send(
		buffer(console),
		[this, self](boost::system::error_code ec, size_t /* length */) {
			if (!ec)
			{
				for (int i = 0; i < MAXHOST; i++)
				{
					if (rs_selected(RS[i]))
					{
						make_shared<ConsoleSession>(RS[i], this)->start();
					}
				}
			}
		});
}

int main(int argc, char *const argv[])
{
	if (argc != 2)
	{
		cerr << "Usage:" << argv[0] << " [port]" << endl;
		return 1;
	}

	try
	{
		unsigned short port = atoi(argv[1]);
		HttpServer server(port);
		global_io_service.run();
	}
	catch (exception &e)
	{
		cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
