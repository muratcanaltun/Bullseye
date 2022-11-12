#include <iostream>
#include <string>
#include <csignal>
#include <vector>
#include <algorithm>

#include <grpcpp/grpcpp.h>
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

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using bullseyeindexservice::IndexReply;
using bullseyeindexservice::IndexRequest;
using bullseyeindexservice::IndexCalc;

volatile std::sig_atomic_t sig_stop;

std::string calculating_index;

mongocxx::instance instance{};
mongocxx::client client{ mongocxx::uri{} };
mongocxx::database db = client["Index"];
mongocxx::collection coll = db["Index_Values"];
mongocxx::collection hist_coll = db["Index_Hist"];

class IndexCalcClient {
public:
	IndexCalcClient(std::shared_ptr<Channel> channel) : stub_(IndexCalc::NewStub(channel)) {}

	std::string sendRequest(std::string index_id)
	{
		IndexRequest request;
		request.set_index_id(index_id);

		IndexReply reply;
		ClientContext context;

		Status status = stub_->sendRequest(&context, request, &reply);

		if (status.ok())
		{
			return std::to_string(reply.index_value());
		}
		else
		{
			std::cout << "Error code " << status.error_code() << ": " << status.error_message() << std::endl;
			return "RPC Failed";
		}
	}
private:
	std::unique_ptr<IndexCalc::Stub> stub_;
};

void RunClient(std::string index_id) 
{
	// init IP adress, response str and client
	std::string target_address("127.0.0.1:50051");
	std::string response;
	IndexCalcClient client(grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials()));

	// get timestamps and response
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	response = client.sendRequest(index_id);
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

	// get the difference between the two timestamps, turn to duration
	std::chrono::duration<double, std::milli> time_spent = t2 - t1;

	// print results
	std::cout << "Fetched for index: " << calculating_index << std::endl;
	std::cout << "Received: " << response << std::endl;
	std::cout << "Calculated in " << time_spent.count() << " ms." << std::endl << std::endl;

	bsoncxx::types::b_date timestamp(std::chrono::system_clock::now());

	if (response.compare("RPC Failed") != 0) {
		auto builder = bsoncxx::builder::stream::document{};
	coll.update_one(document{} << "Name" << calculating_index << finalize, document{} << "$set" << open_document << "Value" << response
		<< "Timestamp" << timestamp << close_document << finalize);

	hist_coll.insert_one(document{} << "Name" << calculating_index << "Value" << response << "Timestamp" << timestamp << finalize);
	}

	

}

static void check_signal(int sig)
{
	if (sig == SIGINT || sig == SIGTERM) {
		sig_stop = sig;
	}
}

int main()
{
	mongocxx::cursor list_cursor = coll.find({});

	std::string index_id;
	std::cout << "Welcome to Bullseye Index Service Mainframe.\nThe indices we currently calculate are:" << std::endl;

	std::vector<std::string> indices;
	for (auto&& doc : list_cursor) {
		bsoncxx::document::element name = doc["Name"];
		indices.push_back(name.get_utf8().value.to_string());
		std::cout << name.get_utf8().value << std::endl;
	}

	do
	{
		std::cout << "\nPlease enter the ID of the index you want calculated:" << std::endl;
		std::getline(std::cin, index_id);
		calculating_index = index_id;
	} while (std::find(indices.begin(), indices.end(), calculating_index) == indices.end());
	

	sig_stop = 0;
	std::signal(SIGINT, &check_signal);
	std::signal(SIGTERM, &check_signal);

	while (sig_stop == 0) {
		RunClient(index_id);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
