#pragma once
#ifndef SERVER_H
#define SERVER_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "BlockingQueue.h"
#include "PageHolder.h"

#include <optional>
#include <string>
#include <type_traits>
#include <thread>
#include <experimental/net>

#include <iostream>

namespace ctas {


	template<class Base, class Session>
	using IsSessionProviderBase = std::enable_if_t<
		std::is_member_function_pointer<decltype(&Base::CreateSession)>::value
		&& std::is_same<decltype(&Base::CreateSession), const std::string (Base::*)(Session&& session)>::value
		&& std::is_member_function_pointer<decltype(&Base::GetSession)>::value
		&& std::is_same<decltype(&Base::GetSession), std::optional<Session*> (Base::*)(const std::string& session_id)>::value
		, Base>;

	template<class Base>
	using IsSession = std::enable_if_t<std::is_move_constructible_v<Base>>;

	template<class WebPage, class Session>
	using IsWebPage = std::enable_if_t<
		std::is_member_function_pointer<decltype(&WebPage::HandleRequest)>::value
		&& std::is_same<decltype(&WebPage::HandleRequest), HttpResponse<Session> (WebPage::*) (HttpRequest<Session>& request)>::value
		, WebPage>;

	template<class SessionProvider, class Session, typename = IsSession<Session>, typename = IsSessionProviderBase<SessionProvider, Session> >
	class Server {
	public:
		template<typename WebPage, typename = IsWebPage<WebPage, Session>>
		void registerPage (const std::string& name) {

			std::unique_ptr<PageHolder<WebPage, Session>> ptr = std::make_unique<PageHolder<WebPage, Session>> ();

			pages.emplace (name, std::move (ptr));
		}

		template<typename WebPage, typename = IsWebPage<WebPage, Session>, typename... Args>
		void registerPage (const std::string& name, Args&&... args) {

			std::unique_ptr<PageHolder<WebPage, Session>> ptr = std::make_unique<PageHolder<WebPage, Session>> (std::forward<Args> (args)...);

			pages.emplace (name, std::move (ptr));
		}

		void Start (unsigned short port) {
			SetupPages ();

			namespace net = std::experimental::net;

			net::io_context ctx;
			net::ip::tcp::acceptor accpt (ctx, net::ip::tcp::endpoint (net::ip::tcp::v4 (), port));

			while (true) { // TODO: End condition
				std::error_code ec;
				net::ip::tcp::socket socket = accpt.accept (ec);
				if (ec) {
					break;
				}
				// handleRead (std::move (socket), ctx);
				queue.Push (std::move (socket));
			}
		}

	private:
		std::unordered_map<std::string, std::unique_ptr<PageHolderBase<Session>>> pages;
		std::vector<std::thread> threads;
		// BlockingQueue<HttpRequest<Session>> queue;
		BlockingQueue<std::experimental::net::ip::tcp::socket> queue;

		SessionProvider session_provider;

		/*
		void handleRead (std::experimental::net::ip::tcp::socket&& socket, std::experimental::net::io_context& ctx) {
			namespace net = std::experimental::net;

			queue.Push (std::move(socket));

			std::string data;
			std::error_code ec;
			net::read_until (socket, net::dynamic_buffer (data), "\r\n\r\n", ec);
			if (ec) {
				// TODO: Handle Error
				return;
			}

			// Parse HTTP Header
			HttpRequest<Session> request (ctx);
			request.Parse (data);

			auto page = pages.find (request.Resource ());
			if (page == pages.end ()) {
				std::error_code ec_write;
				net::write (socket, net::buffer ("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"), ec_write);
				if (ec_write) {
					// TODO: Handle Error
					return;
				}
				socket.close ();
				return;
			}

			// TODO read body if !GET

			PageHolderBase<Session>* holder = page->second.get ();
			request.Page (holder);

			request.Socket (std::move (socket));

			queue.Push (std::move (request));
		}
		*/

		void SetupPages () {
			size_t supported_thread_num = std::thread::hardware_concurrency ();

			for (int i = 0; i < supported_thread_num; ++i) {
				threads.push_back (std::move (std::thread ([&] () {
					while (true) {
						namespace net = std::experimental::net;
						net::ip::tcp::socket&& socket = queue.Pop ();

						// Read stream
						std::string data;
						std::error_code ec;
						net::read_until (socket, net::dynamic_buffer (data), "\r\n\r\n", ec);
						if (ec) {
							// TODO: Handle Error
							return;
						}

						// Parse HTTP Header
						HttpRequest<Session> request;
						request.Parse (data);

						auto page = pages.find (request.Resource ());
						if (page == pages.end ()) {
							std::error_code ec_write;
							net::write (socket, net::buffer ("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"), ec_write);
							if (ec_write) {
								// TODO: Handle Error
								return;
							}
							socket.close ();
							return;
						}

						// TODO read body if !GET

						PageHolderBase<Session>* holder = page->second.get ();

						try {
							std::optional<std::string> session_cookie = request.HeaderField ("Cookie");
							if (session_cookie.has_value ()) {
								std::string& cookie_value = session_cookie.value ();

								size_t equals_pos = cookie_value.find ("id=");
								if (equals_pos != std::string::npos) {
									std::string session_id = cookie_value.substr (equals_pos + 3);
									std::optional<Session*> session = session_provider.GetSession (session_id);
									if (session.has_value ()) {
										request.CurrentSession (session.value ());
									}
								}
							}

							HttpResponse<Session> response = holder->GetResponse (std::move (request));

							auto& fields = response.HeaderFields ();
							fields["Connection"] = " close";
							//fields["Content-Type"] = " text/html";
							if (response.HasSession ()) {
								const std::string new_session_id = session_provider.CreateSession (response.MoveSession ());
								fields["Set-Cookie"] = "id=" + new_session_id;
							}

							std::string resp_str = response.toSendString ();
							std::error_code ec_write;
							std::experimental::net::write (socket, std::experimental::net::buffer (resp_str), ec_write);
							if (ec_write) {
								// TODO: Handle Error
								return;
							}
							socket.close ();

						} catch (std::exception& ex) {
							const char* what = ex.what ();
							std::cout << what;
						}
					}
				}
				)
				)
				);
			}
		}
	};
}

#endif // !SERVER_H