#pragma once
#ifndef PAGEHOLDER_H
#define PAGEHOLDER_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include <exception>

template<typename Session>
class PageHolderBase {
	virtual HttpResponse<Session> GetResponse (HttpRequest<Session>&& request) = 0;
};

template<typename Page, typename Session>
class PageHolder : public PageHolderBase<Session> {
public:

	HttpResponse<Session> GetResponse (HttpRequest<Session>&& request) override {
		try {
			HttpResponse<Session> response = page.HandleRequest (request);

			return response;
		} catch (std::exception& ex) {
			const char* what = ex.what ();
			// TODO restart page
			HttpResponse<Session> response;
			response.ResponseCode (RESPONSE_CODE::ERROR_500);
			return response;
		}
	};
private:
	Page page;
	//std::vector<T> buffered_webpages;
	//ThreadPool threadpool;
	//BlockingQueue<HttpRequest> queue;
};

#endif // !PAGEHOLDER_H