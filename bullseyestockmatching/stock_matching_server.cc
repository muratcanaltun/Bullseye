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

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
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
mongocxx::collection stocks_histdb = db["Stock_Hist"];

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

class StockMatchingServiceImplementation final : public StockMatching::Service
{
	Status matchStocks(ServerContext* context, const matchRequest* request, matchReply* reply) override
	{
		auto sell_orders = ordersdb.find(make_document(kvp("order_type", 2)));

		for (auto sell_order : sell_orders) {
			auto buy_orders = ordersdb.find(make_document(kvp("stock_id", sell_order["stock_id"].get_utf8().value.to_string()),
				kvp("order_type", 1), kvp("stock_price", sell_order["stock_price"].get_double().value)));

			if (buy_orders.begin() != buy_orders.end()) {
				for (auto doc : buy_orders) {
					if (!doc.empty()) {
						int change = doc["percentage"].get_double().value * sell_order["order_size"].get_int32().value;
						double price = doc["stock_price"].get_double().value;
						std::string stock_symbol = doc["stock_id"].get_utf8().value.to_string();

						ordersdb.update_one(document{} << "_id" << bsoncxx::oid(doc["_id"].get_oid().value.to_string()) << finalize,
							document{} << "$set" << open_document
							<< "order_size" << doc["order_size"].get_int32().value - change
							<< close_document << finalize);
						ordersdb.update_one(document{} << "_id" << bsoncxx::oid(sell_order["_id"].get_oid().value.to_string()) << finalize,
							document{} << "$set" << open_document
							<< "order_size" << sell_order["order_size"].get_int32().value - change
							<< close_document << finalize);
						stocksdb.update_one(document{} << "stock_symbol" << stock_symbol << finalize,
							document{} << "$set" << open_document
							<< "stock_price" << price << close_document << finalize);

						bsoncxx::types::b_date timestamp(std::chrono::system_clock::now());
						stocks_histdb.insert_one(document{} << "stock_symbol" << stock_symbol << "stock_price" << price << "timestamp" << timestamp << finalize);
					}
				}
			}
		}

		return Status::OK;
	}
};

void RunServer()
{
	std::string server_address("127.0.0.1:50058");
	StockMatchingServiceImplementation service;

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Bullseye Stock Matching Server listening on: " << server_address << std::endl;

	server->Wait();
}

int main()
{
	RunServer();
}
