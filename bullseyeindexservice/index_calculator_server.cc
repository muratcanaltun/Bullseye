#include <grpcpp/grpcpp.h>
#include <string>
#include <sstream>

#include "bullseyeindexservice.grpc.pb.h"
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
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using bullseyeindexservice::IndexReply;
using bullseyeindexservice::IndexRequest;
using bullseyeindexservice::IndexCalc;

mongocxx::instance instance{};
mongocxx::uri uri("mongodb://localhost:27017");
mongocxx::client client(uri);
mongocxx::database db = client["Index"];
mongocxx::collection coll = db["Index_Values"];

mongocxx::database stockDB = client["Stock"];
mongocxx::collection stock_coll = stockDB["Stocks"];

size_t write_to_string(void* contents, size_t size, size_t nmemb, std::string* s);

class IndexCalcServiceImplementation final : public IndexCalc::Service
{

	Status sendRequest(ServerContext* context, const IndexRequest* request, IndexReply* reply) override
	{
		std::string index_name = request->index_id();
		std::map<std::string, std::pair<double, int>> stocks;

		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		bsoncxx::stdx::optional<bsoncxx::document::value> index_result = coll.find_one(document{} << "Name" << index_name << finalize);
		auto index_view = index_result->view();

		std::string calculation_type = index_view["Type"].get_utf8().value.to_string();
		double index_divisor = index_view["Divisor"].get_double();

		std::istringstream iss(index_view["Symbols"].get_utf8().value.to_string());

		do {
			std::string stock_name;
			iss >> stock_name;

			if (stock_name.empty()) {
				break;
			}

			bsoncxx::stdx::optional<bsoncxx::document::value> stock = stock_coll.find_one(make_document(kvp("stock_symbol", stock_name)));
			
			auto stock_view = stock->view();

			stocks[stock_name] = std::make_pair(stock_view["stock_price"].get_double(), stock_view["stock_volume"].get_int32().value);

		} while (iss);

		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> time_spent = t2 - t1;
		std::cout << "Fetched stock data for " << index_name << " in " << time_spent.count() << " ms." << std::endl;

		double price = 0;

		for (auto const& p : stocks) {
			if (calculation_type.compare("Price Weighted") == 0) {
				price += p.second.first;
			}
			else if (calculation_type.compare("Capitalization Weighted") == 0) {
				price += p.second.first * p.second.second;
			}
			else if (calculation_type.compare("Equal Weighted") == 0) {
				price += p.second.first * (1.0 / stocks.size());
			}
			else if (calculation_type.compare("Capped") == 0) {
				double shares = p.second.second;

				if (index_view["Limit"].get_double() < p.second.second) {
					shares = index_view["Limit"].get_double();
				}

				price += p.second.first * shares;
			}
		}

		price = price / index_divisor;

		reply->set_index_value(price);
		return Status::OK;
	}
};

void RunServer()
{
	std::string server_address("127.0.0.1:50051");
	IndexCalcServiceImplementation service;

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Bullseye Index Service listening on: " << server_address << std::endl;

	server->Wait();
}

size_t write_to_string(void* contents, size_t size, size_t nmemb, std::string* s)
{
	size_t newLength = size * nmemb;
	try
	{
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc& e)
	{
		return 0;
	}
	return newLength;
}

int main()
{
	RunServer();
}
