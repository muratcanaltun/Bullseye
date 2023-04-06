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

#define STOCKS_SIZE 14

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
mongocxx::collection coll = db["Orders"];

namespace
{
    volatile std::sig_atomic_t sig_stop;
}

std::string stocks[] = {
		"AA", "BB", "CC", "D", "EF",
		"F", "G", "HIJK", "LL", "MM",
		"NO", "PQRS", "UVW", "XYZ"
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
		std::cout << "Starting percentage assignment..." << std::endl;
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		int buy_sizes[STOCKS_SIZE];

		for (int i = 0; i < STOCKS_SIZE; i++) {
			auto buy_orders = coll.find(make_document(kvp("stock_id", stocks[i]), kvp("order_type", 1)));
			int buy_size = 0;

			for (auto doc : buy_orders) {
				if (!doc.empty()) {
					buy_size += doc["order_size"].get_int32().value;
				}
			}

			buy_sizes[i] = buy_size;
		}

		for (int i = 0; i < STOCKS_SIZE; i++) {
			auto buy_orders = coll.find(make_document(kvp("stock_id", stocks[i]), kvp("order_type", 1)));
			
			if (buy_orders.begin() != buy_orders.end()) {
				for (auto doc : buy_orders) {
					if (!doc.empty()) {
						std::cout << "Working on order id " << doc["_id"].get_oid().value.to_string() << std::endl;

						coll.update_one(document{} << "_id" << bsoncxx::oid(doc["_id"].get_oid().value.to_string()) << finalize, 
							document{} << "$set" << open_document 
							<< "percentage" << (double) doc["order_size"].get_int32().value / buy_sizes[i]
							<< close_document << finalize);
					}
				}
			}
		}
		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> time_spent = t2 - t1;
		std::cout << "Completed percentage assignment in " << time_spent.count() << " ms." << std::endl;
	}
}