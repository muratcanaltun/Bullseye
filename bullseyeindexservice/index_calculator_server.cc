#include <grpcpp/grpcpp.h>
#include <string>

#include "bullseyeindexservice.grpc.pb.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

#define CURL_MAX_WRITE_SIZE 32768

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using bullseyeindexservice::IndexReply;
using bullseyeindexservice::IndexRequest;
using bullseyeindexservice::IndexCalc;

mongocxx::instance instance{};
mongocxx::client client{ mongocxx::uri{} };
mongocxx::database db = client["Index"];
mongocxx::collection coll = db["Index_Values"];

size_t write_to_string(void* contents, size_t size, size_t nmemb, std::string* s);

class IndexCalcServiceImplementation final : public IndexCalc::Service
{

	Status sendRequest(ServerContext* context, const IndexRequest* request, IndexReply* reply) override
	{
		CURL* curl;
		CURLcode res;
		std::string link = "https://query1.finance.yahoo.com/v7/finance/quote?symbols=";
		std::string index_name = request->index_id();

		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		bsoncxx::stdx::optional<bsoncxx::document::value> index_result = coll.find_one(document{} << "Name" << index_name << finalize);
		auto index_view = index_result->view();

		std::string calculation_type = index_view["Type"].get_utf8().value.to_string();
		double index_divisor = index_view["Divisor"].get_double();
		link = link.append(index_view["Symbols"].get_utf8().value.to_string());

		curl = curl_easy_init();
		std::string stringify;

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stringify);

			res = curl_easy_perform(curl);

			curl_easy_cleanup(curl);
		}

		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> time_spent = t2 - t1;
		std::cout << "Fetched stock data for " << index_name << " in " << time_spent.count() << " ms." << std::endl;

		rapidjson::Document doc;
		doc.Parse<0>(stringify.c_str());

		assert(doc.IsObject());

		rapidjson::Value& quoteResponse = doc["quoteResponse"];
		rapidjson::Value& result = quoteResponse["result"];

		assert(result.IsArray());

		double price = 0;

		for (auto const& p : quoteResponse["result"].GetArray()) {
			if (calculation_type.compare("Price Weighted") == 0) {
				price += p["regularMarketPrice"].GetDouble();
			}
			else if (calculation_type.compare("Capitalization Weighted") == 0) {
				price += p["regularMarketPrice"].GetDouble() * p["sharesOutstanding"].GetDouble();
			}
			else if (calculation_type.compare("Equal Weighted") == 0) {
				price += p["regularMarketPrice"].GetDouble() * (1.0 / quoteResponse["result"].GetArray().Size());
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
