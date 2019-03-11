#pragma once
#ifndef PAGEHOLDER_H
#define PAGEHOLDER_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include <exception>

template<typename Session>
class PageHolderBase {
public:
	virtual HttpResponse<Session> GetResponse (HttpRequest<Session>&& request) = 0;
};

template<typename Page, typename Session>
class PageHolder : public PageHolderBase<Session> {
public:

	PageHolder () = default;

	template<typename... Args>
	PageHolder (Args&&... args) : page(std::forward<Args> (args)...) {}

	HttpResponse<Session> GetResponse (HttpRequest<Session>&& request) override {
		try {
			HttpResponse<Session> response = page.HandleRequest (request);

			return response;
		} catch (std::exception& ex) {
			const char* what = ex.what ();

			page = Page();

			HttpResponse<Session> response;
			response.ResponseCode (RESPONSE_CODE::ERROR_500);
			return response;
		}
	};
private:
	Page page;
};

#endif // !PAGEHOLDER_H
