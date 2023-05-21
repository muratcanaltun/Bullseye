#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <vector>

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

#define STOCKS_SIZE 30

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

mongocxx::instance instance{};
mongocxx::uri uri("mongodb://localhost:27017");
mongocxx::client client(uri);
mongocxx::database db = client["Stock"];
mongocxx::collection stocks_coll = db["Stocks"];
mongocxx::collection hist_coll = db["Stock_Hist"];

namespace
{
    volatile std::sig_atomic_t sig_stop;
}

std::string stocks[] = {
		"AA","BB","CC","D","EF","F","G",
		"HIJK","LL","MM","NO","PQRS","UVW",
		"XYZ","XDDD","TSK","IBB","UNK","USD",
		"UST","FRA","PGO","PDF","FB","CL",
		"BTEC","CIVC","REEN","WSTD","IRO"
};

class Order_Data {
public:
	std::string stock_id;
	double stock_price;
	int order_size;

	Order_Data(std::string stock_id, double stock_price, int order_size) {
		this->stock_id = stock_id;
		this->stock_price = stock_price;
		this->order_size = order_size;
	}
};

int main()
{
	while (sig_stop == 0) {
		int stock_index = std::rand() % STOCKS_SIZE;
		std::cout << "generated " << stock_index << std::endl;

		std::string stock_symbol = stocks[stock_index];

		std::cout << "got " << stock_symbol << std::endl;

		bsoncxx::stdx::optional<bsoncxx::document::value> stock = stocks_coll.find_one(document{} << "stock_symbol" << stock_symbol << finalize);
		auto stock_view = stock->view();

		int random = std::rand() % 2;
		std::cout << "generated rand" << std::endl;

		double price = stock_view["stock_price"].get_double().value + random;
		bsoncxx::types::b_date timestamp(std::chrono::system_clock::now());

		stocks_coll.update_one(document{} << "stock_symbol" << stock_symbol << finalize, document{} << "$set" << open_document << "stock_price" << price << close_document << finalize);
		hist_coll.insert_one(document{} << "stock_symbol" << stock_symbol << "stock_price" << price << "timestamp" << timestamp << finalize);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}