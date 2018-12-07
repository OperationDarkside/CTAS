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

		return resp;
	}
};

int main () {
	ctas::Server<SimpleSessionProvider, SimpleSession> server;

	server.registerPage<MyFirstPage> ("/");

	server.Start ();

	return 0;
}