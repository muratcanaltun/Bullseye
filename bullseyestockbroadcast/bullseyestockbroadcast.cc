#include <iostream>
#include <string>
#include <chrono>

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


void handle_get(web::http::http_request request)
{
    mongocxx::uri uri("mongodb://localhost:27017");
    mongocxx::client client(uri);
    mongocxx::database db = client["Stock"];
    mongocxx::collection coll = db["Stocks"];

    web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(utility::conversions::to_string_t("Access-Control-Allow-Origin"), utility::conversions::to_string_t("*"));
    response.headers().add(utility::conversions::to_string_t("Access-Control-Allow-Methods"), utility::conversions::to_string_t("GET"));
    response.headers().add(utility::conversions::to_string_t("Access-Control-Allow-Headers"), utility::conversions::to_string_t("access-control-allow-headers,access-control-allow-methods,access-control-allow-origin"));

    auto path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));
    std::string splitted(utility::conversions::to_utf8string(path[0]));

    std::cout << "Handling GET " + splitted << std::endl;
    auto answer = web::json::value::object();

    mongocxx::cursor cursor = coll.find(document{} << "stock_symbol" << splitted << finalize);

    std::vector<web::json::value> values;
    for (auto doc : cursor) {
        web::json::value object = web::json::value::object();
        
        object[utility::conversions::to_string_t("stock_symbol")] = web::json::value::string(utility::conversions::to_string_t(doc["stock_symbol"].get_utf8().value.to_string()));
        object[utility::conversions::to_string_t("stock_price")] = web::json::value::number((doc["stock_price"].get_double().value));
        object[utility::conversions::to_string_t("stock_volume")] = web::json::value::number((doc["stock_volume"].get_int32().value));
        
        values.push_back(object);
    }
    answer[utility::conversions::to_string_t("response")] = web::json::value::array(values);

    response.set_body(answer);
    request.reply(response);
}

void handle_hist_get(web::http::http_request request)
{
    mongocxx::uri uri("mongodb://localhost:27017");
    mongocxx::client client(uri);
    mongocxx::database db = client["Stock"];
    mongocxx::collection coll = db["Stock_Hist"];

    web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(utility::conversions::to_string_t("Access-Control-Allow-Origin"), utility::conversions::to_string_t("*"));
    response.headers().add(utility::conversions::to_string_t("Access-Control-Allow-Methods"), utility::conversions::to_string_t("GET"));
    response.headers().add(utility::conversions::to_string_t("Access-Control-Allow-Headers"), utility::conversions::to_string_t("access-control-allow-headers,access-control-allow-methods,access-control-allow-origin"));

    auto path = web::http::uri::split_path(web::http::uri::decode(request.relative_uri().path()));
    std::string splitted(utility::conversions::to_utf8string(path[0]));

    std::cout << "Handling GET " + splitted << std::endl;
    auto answer = web::json::value::object();

    mongocxx::cursor cursor = coll.find(document{} << "stock_symbol" << splitted << finalize);

    std::vector<web::json::value> values;
    for (auto doc : cursor) {
        web::json::value object = web::json::value::object();

        object[utility::conversions::to_string_t("stock_symbol")] = web::json::value::string(utility::conversions::to_string_t(doc["stock_symbol"].get_utf8().value.to_string()));
        object[utility::conversions::to_string_t("stock_price")] = web::json::value::number((doc["stock_price"].get_double().value));
        object[utility::conversions::to_string_t("timestamp")] = web::json::value::number((doc["timestamp"].get_date().value.count()));

        values.push_back(object);
    }
    answer[utility::conversions::to_string_t("response")] = web::json::value::array(values);

    response.set_body(answer);
    request.reply(response);
}

int main()
{
    web::http::experimental::listener::http_listener listener(utility::conversions::to_string_t("http://localhost:9991/Stocks/"));
    web::http::experimental::listener::http_listener hist_listener(utility::conversions::to_string_t("http://localhost:9991/Stocks_Hist/"));

    listener.support(web::http::methods::GET, handle_get);
    hist_listener.support(web::http::methods::GET, handle_hist_get);

    try
    {
        listener
            .open()
            .then([&listener]() {std::cout << "Started to Listen" << std::endl; })
            .wait();
        hist_listener
            .open()
            .then([&hist_listener]() {std::cout << "Started to Listen HIST" << std::endl; })
            .wait();

        while (true);
    }
    catch (std::exception const& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
