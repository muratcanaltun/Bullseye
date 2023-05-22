#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <vector>

#undef U
// If 'U' isn't undefined, the code doesn't compile
#include <grpcpp/grpcpp.h>
#include "bullseyestockmatching.grpc.pb.h"
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

#define STOCKS_SIZE 30

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using bullseyestockmatching::StockMatching;
using bullseyestockmatching::matchReply;
using bullseyestockmatching::matchRequest;

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
mongocxx::collection ordersdb = db["Orders"];
mongocxx::collection stocksdb = db["Stocks"];

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

class StockMatchingClient {
public:
	StockMatchingClient(std::shared_ptr<Channel> channel) : stub_(StockMatching::NewStub(channel)) {}

	std::string matchStocks()
	{
		matchRequest request;

		request.set_error("OK");

		matchReply reply;
		ClientContext context;

		Status status = stub_->matchStocks(&context, request, &reply);

		if (status.ok())
		{
			return "Percentage matching OK.";
		}
		else
		{
			std::cout << "Error code " << status.error_code() << ": " << status.error_message() << std::endl;
			return "RPC Failed";
		}
	}

private:
	std::unique_ptr<StockMatching::Stub> stub_;
};

void RunClient()
{
	// init IP adress, response str and client
	std::string target_address("127.0.0.1:50058");
	std::string response;
	StockMatchingClient client(grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials()));

	std::map<std::pair<std::string, double>, int> buy_sizes_map;

	//int buy_sizes[STOCKS_SIZE];

	for (int i = 0; i < STOCKS_SIZE; i++) {
		auto buy_orders = ordersdb.find(make_document(kvp("stock_id", stocks[i]), kvp("order_type", 1)));

		for (auto doc : buy_orders) {
			if (!doc.empty()) {
				if (!buy_sizes_map.count(std::make_pair(doc["stock_id"].get_utf8().value.to_string(), doc["stock_price"].get_double().value))) {
					buy_sizes_map[std::make_pair(doc["stock_id"].get_utf8().value.to_string(), doc["stock_price"].get_double().value)] = doc["order_size"].get_int32().value;
				}
				else {
					buy_sizes_map[std::make_pair(doc["stock_id"].get_utf8().value.to_string(), doc["stock_price"].get_double().value)] += doc["order_size"].get_int32().value;
				}
			}
		}
	}

	for (auto p : buy_sizes_map) {
		std::cout << p.first.first << ": " << p.second << std::endl;
	}

	for (int i = 0; i < STOCKS_SIZE; i++) {
		auto buy_orders = ordersdb.find(make_document(kvp("stock_id", stocks[i]), kvp("order_type", 1)));

		if (buy_orders.begin() != buy_orders.end()) {
			for (auto doc : buy_orders) {
				if (!doc.empty()) {
					std::cout << "Working on order id " << doc["_id"].get_oid().value.to_string() << std::endl;

					if (doc["order_size"].get_int32().value <= 0) {
						ordersdb.find_one_and_delete(make_document(kvp("_id", doc["_id"].get_oid().value)));
					}
					else {
						ordersdb.update_one(document{} << "_id" << bsoncxx::oid(doc["_id"].get_oid().value.to_string()) << finalize,
							document{} << "$set" << open_document
							<< "percentage" << (double)doc["order_size"].get_int32().value / 
							buy_sizes_map[std::make_pair(doc["stock_id"].get_utf8().value.to_string(), doc["stock_price"].get_double().value)]
							<< close_document << finalize);
					}
				}
			}
		} 
	}

	// get timestamps and response
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	response = client.matchStocks();
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

	// get the difference between the two timestamps, turn to duration
	std::chrono::duration<double, std::milli> time_spent = t2 - t1;

	// print results
	std::cout << "Matched stocks in " << time_spent.count() << " ms." << std::endl << std::endl;

	bsoncxx::types::b_date timestamp(std::chrono::system_clock::now());
}

int main()
{
	while (sig_stop == 0) {
		RunClient();
	}
}