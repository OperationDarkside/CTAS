#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "Server.h"

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

	const std::string& CreateSession () {
		return "";
	}

	/*const std::string& CreateSession (SimpleSession&& session) {
		return "";
	}*/

	std::optional<SimpleSession*> GetSession (const std::string& session_id) {
		return nullptr;
	}

private:
	int next_session_id = 0;

	std::unordered_map<std::string, SimpleSession> sessions;
};

int main () {

	ctas::Server<SimpleSessionProvider, SimpleSession> server;

	server.Start ();

	return 0;
}