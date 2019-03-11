#pragma once
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <optional>
#include <unordered_map>

#include <experimental/net>

template<typename Session>
class PageHolderBase;

enum class REQUEST_METHOD {
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE
};

template<typename Session>
class HttpRequest {
public:
	HttpRequest (/*std::experimental::net::io_context& ctx*/) /*: sock(ctx)*/ {

	}

	REQUEST_METHOD& Method () {
		return method;
	};
	std::string& Resource () {
		return resource;
	};
	std::string& Version () {
		return version;
	};
	std::optional<std::string> HeaderField (const std::string& fieldname) {
		auto element = header_fields.find (fieldname);
		if (element == header_fields.end ()) {
			return std::nullopt;
		}
		return element->second;
	};
	std::optional<std::string> GET (const std::string& fieldname) {
		auto element = get_fields.find (fieldname);
		if (element == get_fields.end ()) {
			return std::nullopt;
		}
		return element->second;
	};
	std::optional<std::string> POST (const std::string& fieldname) {
		auto element = post_fields.find (fieldname);
		if (element == post_fields.end ()) {
			return std::nullopt;
		}
		return element->second;
	};

	/*PageHolderBase<Session>* Page () {
		return page;
	};*/

	/*void Page (PageHolderBase<Session>* _page) {
		page = _page;
	};*/

	bool HasSession () {
		return session != nullptr;
	};

	Session* CurrentSession () {
		return session;
	};

	void CurrentSession (Session* _session) {
		session = _session;
	};

	/*std::experimental::net::ip::tcp::socket& Socket () {
		return sock;
	};
	void Socket (std::experimental::net::ip::tcp::socket&& socket) {
		sock = std::move (socket);
	};*/
	/*
	Net::Sockets::Socket& Socket () {
		return sock;
	};
	void Socket (Net::Sockets::Socket socket) {
		sock = socket;
	};
	*/

	void ParseGetFields () {
		if (get_str.empty ()) {
			return;
		}

		if (get_str.find ('&') == std::string::npos) {
			// only one param
			size_t equals_pos = get_str.find ('=');
			if (equals_pos != std::string::npos) {
				std::string key = get_str.substr (0, equals_pos);
				std::string val;
				if (get_str.length () > (equals_pos + 1)) {
					val = get_str.substr (equals_pos + 1);
				}
				get_fields[UriDecode (key)] = UriDecode (val);
			} else {
				get_fields[UriDecode (get_str)] = "";
			}
			return;
		}

		size_t new_pos = 0;
		size_t last_pos = 0;
		size_t len = get_str.length ();
		std::string line;

		while ((new_pos = get_str.find ('&', last_pos)) != std::string::npos) {
			line = get_str.substr (last_pos, new_pos);

			size_t equals_pos = line.find ('=');
			if (equals_pos != std::string::npos) {
				std::string key = line.substr (0, equals_pos);
				std::string val;
				if (line.length () > (equals_pos + 1)) {
					val = line.substr (equals_pos + 1);
				}
				get_fields[UriDecode (key)] = UriDecode (val);
			} else {
				get_fields[UriDecode (line)] = "";
			}

			if (len < (new_pos + 2)) {
				break;
			}
			last_pos = new_pos + 1;
		}
	};
	bool Parse (const std::string& value) {
		size_t len = value.length ();

		std::string line;
		std::vector<std::string> fields;

		// FIRST LINE
		size_t first_line_end = value.find ("\r\n", 0);
		if (first_line_end == std::string::npos) {
			return false;
		}
		line = value.substr (0, first_line_end);
		if (!ParseFirstLine (line)) {
			// First line parse error
			return false;
		}

		if (len < (first_line_end + 3)) {
			return false;
		}

		// HEADER FIELDS
		size_t header_end_pos = value.find ("\r\n\r\n", first_line_end);
		if (header_end_pos == std::string::npos) {
			return false;
		}

		std::string header_str = value.substr (first_line_end + 2, header_end_pos - (first_line_end + 2));
		size_t header_str_len = header_str.length ();
		size_t new_pos = 0;
		size_t last_pos = 0;
		while ((new_pos = header_str.find ("\r\n", last_pos)) != std::string::npos) {
			line = header_str.substr (last_pos, new_pos - last_pos);
			fields.push_back (line);
			if (header_str_len < (new_pos + 3)) {
				break;
			}
			last_pos = new_pos + 2;
		}
		// catch the last field
		if (last_pos != std::string::npos && last_pos < header_str_len) {
			line = header_str.substr (last_pos);
			fields.push_back (line);
		}

		for (auto& field : fields) {
			auto colon_pos = field.find (':');
			if (colon_pos == std::string::npos) {
				// is not a valid field
				header_fields[field] = "";
			}
			std::string key = field.substr (0, colon_pos);

			if (field.length () < (colon_pos + 2)) {
				header_fields[key] = "";
			}
			std::string val = field.substr (colon_pos + 1);

			header_fields[key] = val;
		}

		return true;
	};

private:
	REQUEST_METHOD method;
	std::string resource;
	std::string get_str;
	std::string version;
	std::unordered_map<std::string, std::string> header_fields;
	std::unordered_map<std::string, std::string> get_fields;
	std::unordered_map<std::string, std::string> post_fields;

	//PageHolderBase<Session>* page = nullptr;
	Session* session = nullptr;

	//std::experimental::net::ip::tcp::socket sock; // TODO: solve init problem
	//Net::Sockets::Socket sock;

	bool ParseFirstLine (const std::string& line) {
		unsigned short blank_count = 0;
		std::string meth;
		std::string resrc;
		std::string vers;

		for (const char& c : line) {
			if (c == ' ') {
				++blank_count;
				if (blank_count > 2) {
					return false;
				}
				continue;
			}
			if (blank_count == 0) {
				meth += c;
			}
			if (blank_count == 1) {
				resrc += c;
			}
			if (blank_count == 2) {
				vers += c;
			}
		}

		// METHOD
		if (meth == "GET") {
			method = REQUEST_METHOD::HTTP_GET;
		} else if (meth == "POST") {
			method = REQUEST_METHOD::HTTP_POST;
		} else if (meth == "PUT") {
			method = REQUEST_METHOD::HTTP_PUT;
		} else if (meth == "DELETE") {
			method = REQUEST_METHOD::HTTP_DELETE;
		}

		// RESOURCE
		size_t questionmark_pos = resrc.find ('?');
		if (questionmark_pos != std::string::npos && questionmark_pos < (resrc.length () + 2)) {
			resource = resrc.substr (questionmark_pos);
			get_str = resrc.substr (questionmark_pos + 1);
		} else {
			resource = std::move (resrc);
		}

		// VERSION
		size_t slash_pos = vers.find ('/');
		if (slash_pos != std::string::npos && slash_pos < vers.length ()) {
			version = vers.substr (slash_pos + 1);
		}

		return true;
	};
	std::string UriDecode (const std::string & sSrc) {
		// Note from RFC1630: "Sequences which start with a percent
		// sign but are not followed by two hexadecimal characters
		// (0-9, A-F) are reserved for future extension"

		const unsigned char * pSrc = (const unsigned char *) sSrc.c_str ();
		const int SRC_LEN = sSrc.length ();
		const unsigned char * const SRC_END = pSrc + SRC_LEN;
		// last decodable '%' 
		const unsigned char * const SRC_LAST_DEC = SRC_END - 2;

		char * const pStart = new char[SRC_LEN];
		char * pEnd = pStart;

		while (pSrc < SRC_LAST_DEC) {
			if (*pSrc == '%') {
				char dec1, dec2;

				dec1 = *(pSrc + 1) & 0xF0 >> 4;
				dec2 = *(pSrc + 2) & 0x0F >> 0;

				if (-1 != dec1 && -1 != dec2) {
					*pEnd++ = (dec1 << 4) + dec2;
					pSrc += 3;
					continue;
				}
			}

			*pEnd++ = *pSrc++;
		}

		// the last 2- chars
		while (pSrc < SRC_END)
			*pEnd++ = *pSrc++;

		std::string sResult (pStart, pEnd);
		delete[] pStart;
		return sResult;
	};
};
#endif // !HTTPREQUEST_H
