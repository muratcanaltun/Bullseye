#include <iostream>
#include <string>
#include <csignal>

#include <grpcpp/grpcpp.h>
#include "bullseyeindexsupervisor.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using bullseyeindexsupervisor::IndexReply;
using bullseyeindexsupervisor::IndexRequest;
using bullseyeindexsupervisor::IndexCheck;

namespace
{
	volatile std::sig_atomic_t sig_stop;
}

double prev_vals[2];
std::string checking_index;
double index_difference;
int state = 1;

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

	int current_val = std::atof(response.c_str());
	index_difference = (current_val - prev_vals[state - 1]) / prev_vals[state - 1] * 100;

	// print results
	std::cout << "Fetched for index: " << checking_index << std::endl;
	std::cout << "Received: " << response << std::endl;
	std::cout << "Difference (%): " << index_difference << std::endl << std::endl;

	if (index_difference > 10) {
		std::cout << "WARNING! PRICE DIFFERENCE OVER 10%!" << std::endl;
	}

	prev_vals[state - 1] = current_val;
}

static void check_signal(int sig)
{
	if (sig == SIGINT || sig == SIGTERM) {
		sig_stop = sig;
	}
}

int main()
{
	prev_vals[0] = 1;
	prev_vals[1] = 1;
	checking_index = "DJIA";

	sig_stop = 0;
	std::signal(SIGINT, &check_signal);
	std::signal(SIGTERM, &check_signal);

	while (sig_stop == 0) {
		RunClient(checking_index);
		switch (state)
		{
		case 2:
			checking_index = "DJIA";
			state = 1;
			break;
		default:
			checking_index = "S&P 500";
			state = 2;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}