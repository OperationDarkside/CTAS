#pragma once
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <optional>
#include <unordered_map>

enum class RESPONSE_CODE {
	OK_200,
	REDIRECT_301,
	NOT_FOUND_404,
	TEAPOT_418,
	ERROR_500
};

template<typename Session>
class HttpResponse {
public:
	//HttpResponse ();
	//~HttpResponse ();

	RESPONSE_CODE ResponseCode () {
		return response_code;
	};
	void ResponseCode (RESPONSE_CODE code) {
		response_code = code;
	};

	std::string& Body () {
		return body;
	};
	void Body (std::string body) {
		this->body = body;
	};

	std::unordered_map<std::string, std::string>& HeaderFields () {
		return header_fields;
	};

	std::string toSendString () {
		std::string res = "HTTP/" + version;

		switch (response_code) {
		case RESPONSE_CODE::OK_200:
			res += " 200 OK";
			break;
		case RESPONSE_CODE::NOT_FOUND_404:
			res += " 404 Not Found";
			break;
		}
		res += "\r\n";

		if (header_fields.size () > 0) {
			for (auto field : header_fields) {
				res += field.first + ':' + field.second + "\r\n";
			}
			res.erase (res.length () - 2);
		}

		res += "\r\n\r\n" + body;

		return res;
	};

	bool HasSession () {
		return session.has_value ();
	}

	Session& CreateSession () {
		if (!session) {
			session = Session ();
		}
		return session.value ();
	};

	Session&& MoveSession () {
		return std::move (session.value ());
	};

private:
	RESPONSE_CODE response_code = RESPONSE_CODE::OK_200;
	std::string version = "1.1";
	std::string body;
	std::unordered_map<std::string, std::string> header_fields;

	std::optional<Session> session = std::nullopt;
};

#endif // !HTTPRESPONSE_H