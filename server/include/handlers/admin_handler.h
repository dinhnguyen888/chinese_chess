#pragma once
#include <boost/beast/http.hpp>

namespace handlers {
    boost::beast::http::response<boost::beast::http::string_body> 
    handle_admin_request(const boost::beast::http::request<boost::beast::http::string_body>& req);
}
