#include <iostream>
#include <string>
#include <csignal>

#include <grpcpp/grpcpp.h>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include "bullseyeindexsupervisor.grpc.pb.h"

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using bullseyeindexsupervisor::IndexReply;
using bullseyeindexsupervisor::IndexRequest;
using bullseyeindexsupervisor::IndexCheck;

mongocxx::instance instance{};
mongocxx::client client{ mongocxx::uri{} };
mongocxx::database db = client["Index"];
mongocxx::collection coll = db["Index_Values"];

namespace
{
	volatile std::sig_atomic_t sig_stop;
}

double prev_vals[2] = {1,1};
std::string checking_index = "S&P 500";
double index_difference;
int state = 1;
bool first_time = true;

class IndexCheckClient {
public:
	IndexCheckClient(std::shared_ptr<Channel> channel) : stub_(IndexCheck::NewStub(channel)) {}

	std::string sendRequest(std::string index_name)
	{
		IndexRequest request;
		request.set_index_name(index_name);

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
	std::unique_ptr<IndexCheck::Stub> stub_;
};

void RunClient(std::string index_name)
{
	// init IP adress, response str and client
	std::string target_address("127.0.0.1:50055");
	std::string response;
	IndexCheckClient client(grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials()));

	response = client.sendRequest(index_name);

	double current_val = std::stod(response.c_str());
	index_difference = (current_val - prev_vals[state - 1]) / prev_vals[state - 1] * 100;

	// print results
	std::cout << "Fetched for index: " << checking_index << std::endl;
	std::cout << "Received: " << response << std::endl;
	std::cout << "Difference (%): " << index_difference << std::endl << std::endl;

	if (abs(index_difference) > 10 && !first_time) {
		std::cout << "WARNING! PRICE DIFFERENCE OVER 10%!" << std::endl;
	}

	prev_vals[state - 1] = current_val;
	first_time = false;
}

static void check_signal(int sig)
{
	if (sig == SIGINT || sig == SIGTERM) {
		sig_stop = sig;
	}
}

int main()
{
	sig_stop = 0;
	std::signal(SIGINT, &check_signal);
	std::signal(SIGTERM, &check_signal);

	mongocxx::cursor list_cursor = coll.find({});

	std::string index_id;
	std::cout << "Welcome to Bullseye Index Supervisor Mainframe.\nThe indices you can oversee are:" << std::endl;

	std::vector<std::string> indices;
	for (auto&& doc : list_cursor) {
		bsoncxx::document::element name = doc["Name"];
		indices.push_back(name.get_utf8().value.to_string());
		std::cout << name.get_utf8().value << std::endl;
	}

	do
	{
		std::cout << "\nPlease enter the ID of the index you want to oversee:" << std::endl;
		std::getline(std::cin, index_id);
		checking_index = index_id;
	} while (std::find(indices.begin(), indices.end(), checking_index) == indices.end());

	while (sig_stop == 0) {
		RunClient(checking_index);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}
