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
		&& std::is_same<decltype(&Base::CreateSession), const std::string& (Base::*)()>::value
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

		void Start () {
			namespace net = std::experimental::net;

			net::io_context ctx;
			net::ip::tcp::socket socket (ctx);
			net::ip::tcp::acceptor accpt (ctx, net::ip::tcp::endpoint (net::ip::tcp::v4 (), 1337));
			
			while (true) {
				std::error_code ec;
				socket = accpt.accept (ec);
				if (ec) {
					break;
				}
				std::string data;
				net::read_until (socket, net::dynamic_buffer(data), "\r\n\r\n");

				std::cout << data;
				
			}
		}

	private:
		std::unordered_map<std::string, std::unique_ptr<PageHolderBase<Session>>> pages;
		std::vector<std::thread> threads;
		BlockingQueue<HttpRequest<Session>> queue;

		SessionProvider session_provider;
		/*
		void SetupPages () {
			size_t supported_thread_num = std::thread::hardware_concurrency ();

			for (int i = 0; i < supported_thread_num; ++i) {
				threads.push_back (std::move (std::thread ([&] () {
					while (true) {
						HttpRequest<Session> request = queue.Pop ();

						try {
							//HttpResponse response = wp->HandleRequest (request);
							PageHolderBase<Session>* page = request.Page ();
							if (page == nullptr) {
								continue;
							}

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

							HttpResponse<Session> response = page->GetResponse (std::move (request));

							auto& fields = response.HeaderFields ();
							fields["Connection"] = " close";
							fields["Content-Type"] = " text/html";
							if (response.HasSession ()) {
								const std::string& new_session_id = session_provider.CreateSession (response.MoveSession ());
								fields["Set-Cookie"] = "id=" + new_session_id;
							}

							// response = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n" + response;

							Net::Sockets::Socket& socket = request.Socket ();

							std::string resp_str = response.toSendString ();

							socket.Send (resp_str.c_str ());
							socket.Disconnect ();
						} catch (std::exception& ex) {
							const char* what = ex.what ();
						}
						// socket->Disconnect();
					}
				}
				)
				)
				);
			}
		}
		void HandleReads (std::list<Net::Sockets::Socket>& clientSockets, std::vector<Net::Sockets::Socket*>& readSockets) {
			std::vector<Net::Sockets::Socket*> eraseSockets;

			for (Net::Sockets::Socket* s : readSockets) {
				String request_str;
				while (true) {
					std::array<char, 100> recvBuffer;

					int bytesrecvd = s->Receive (recvBuffer);

					if (bytesrecvd < 1) {
						switch (bytesrecvd) {
						case 0:
							Console::WriteLine ("Active Client Disconnect");
							// ERASE AFTER DISCONNECT
							eraseSockets.push_back (s);
							break;
						case SOCKET_ERROR:
							Console::WriteLine ("SOCKET_ERROR in Receive");
							break;
						}
						break;
					}

					String recvBufferStr = recvBuffer;

					request_str += recvBufferStr;

					if (bytesrecvd != 100) {
						break;
					}
				}

				if (request_str[0] == 0) {
					Console::WriteLine ("Nothing received");
					continue;
				}

				// Parse HTTP Header
				HttpRequest<Session> request;
				request.Parse (request_str.GetStringValue ());
				request.Socket (*s);

				auto& page = pages.find (request.Resource ());
				if (page == pages.end ()) {
					s->Send ("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
					s->Disconnect ();
					eraseSockets.push_back (s);
					// ERASE AFTER DISCONNECT
					Console::WriteLine (dnc::String ("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n" + request.Resource ()));
					continue;
				}

				PageHolderBase<Session>* holder = page->second.get ();
				request.Page (holder);



				queue.Push (std::move (request));
				//holder->GetResponse (std::move (request));
				//response = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n" + response;

				//Console::WriteLine(response);

				//s->Send(response.GetStringValue().c_str());
				//s->Disconnect();
				// ERASE AFTER DISCONNECT
				eraseSockets.push_back (s);
			}
			for (Net::Sockets::Socket* s : eraseSockets) {
				clientSockets.erase (std::remove (clientSockets.begin (), clientSockets.end (), *s), clientSockets.end ());
				//readSockets.erase(std::remove(readSockets.begin(), readSockets.end(), s), readSockets.end());
			}
		}
		*/
	};
}

#endif // !SERVER_H