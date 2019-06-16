#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "Server.h"

#include <queue>
#include <unordered_map>


class SimpleSession {
public:
	SimpleSession () {}
	~SimpleSession () {}

	bool IsSet (const std::string& key) {
		return true;
	}

	//std::string& GetValue (const std::string& key) { return ""; }

	void Insert (std::string key, std::string value) {}

private:
	std::unordered_map<std::string, std::string> values;
};

class SimpleSessionProvider {
public:

	const std::string CreateSession (SimpleSession&& session) {
		std::string session_id = std::to_string(++next_session_id);

		sessions.emplace (session_id, std::move(session));
		return session_id;
	}

	/*const std::string& CreateSession (SimpleSession&& session) {
		return "";
	}*/

	std::optional<SimpleSession*> GetSession (const std::string& session_id) {
		return std::nullopt;
	}

private:
	int next_session_id = 0;

	std::unordered_map<std::string, SimpleSession> sessions;
};

class MyFirstPage {
public:

	HttpResponse<SimpleSession> HandleRequest (HttpRequest<SimpleSession>& request) {
		HttpResponse<SimpleSession> resp;

		auto& headers = resp.HeaderFields ();
		headers["Content-Type"] = "text/html";
		resp.Body (R"(<!DOCTYPE html>
<html>
<body>

<h2>HTML Forms</h2>

<form action="/Receiver" method="POST">
  First name:<br>
  <input type="text" name="firstname" value="Mickey">
  <br>
  Last name:<br>
  <input type="text" name="lastname" value="Mouse">
  <br><br>
  <input type="submit" value="Submit">
</form> 

<p>If you click the "Submit" button, the form-data will be sent to a page called "/action_page.php".</p>

</body>
</html>)");

		return resp;
	}
};

class Receiver {
public:

	HttpResponse<SimpleSession> HandleRequest (HttpRequest<SimpleSession>& request) {
		HttpResponse<SimpleSession> resp;

		auto& headers = resp.HeaderFields ();
		headers["Content-Type"] = "text/html";

		const std::string& body = request.Body();

		std::cout << body << std::endl;

		return resp;
	}
};

int main () {
	ctas::Server<SimpleSessionProvider, SimpleSession> server;

	server.registerPage<MyFirstPage> ("/");
	server.registerPage<Receiver> ("/Receiver");

	server.Start (1337);

	return 0;
}
