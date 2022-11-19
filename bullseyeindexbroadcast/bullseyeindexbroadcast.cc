#include <iostream>
#include <string>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#undef U
// If 'U' isn't undefined, the code doesn't compile
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

mongocxx::instance instance{};
mongocxx::client client{ mongocxx::uri{} };
mongocxx::database db = client["Index"];
mongocxx::collection coll = db["Index_Values"];
mongocxx::collection hist_coll = db["Index_Hist"];

void handle_get(web::http::http_request request)
{
    auto path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));
    std::string splitted(utility::conversions::to_utf8string(path[0]));

    std::cout << "Handling GET " + splitted << std::endl;
    auto answer = web::json::value::object();

    mongocxx::cursor cursor = hist_coll.find(document{} << "Name" << splitted << finalize);

    std::vector<web::json::value> values;
    for (auto doc : cursor) {
        values.push_back(web::json::value::string(utility::conversions::to_string_t(bsoncxx::to_json(doc))));
    }
    answer[utility::conversions::to_string_t("response")] = web::json::value::array(values);

    request.reply(web::http::status_codes::OK, answer);
}

int main()
{
    web::http::experimental::listener::http_listener listener(L"http://localhost:8080/Indices/");

    listener.support(web::http::methods::GET, handle_get);

    try
    {
        listener
            .open()
            .then([&listener]() {std::cout << "Started to Listen" << std::endl; })
            .wait();

        while (true);
    }
    catch (std::exception const& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}