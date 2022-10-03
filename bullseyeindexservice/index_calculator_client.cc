#include <grpcpp/grpcpp.h>
#include <string>
#include "bullseyeindexservice.grpc.pb.h"
#include <csignal>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using bullseyeindexservice::IndexReply;
using bullseyeindexservice::IndexRequest;
using bullseyeindexservice::IndexCalc;

namespace
{
	volatile std::sig_atomic_t sig_stop;
}

class IndexCalcClient {
public:
	IndexCalcClient(std::shared_ptr<Channel> channel) : stub_(IndexCalc::NewStub(channel)) {}

	std::string sendRequest(int index_id)
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

		// if statement
	}
private:
	std::unique_ptr<IndexCalc::Stub> stub_;
};

void RunClient(int index_id) 
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
	std::cout << "Calculated in " << time_spent.count() << " ms." << std::endl;
	std::cout << "Fetched for ID: " << index_id << std::endl;
	std::cout << "Received: " << response << std::endl;
}

static void check_signal(int sig)
{
	if (sig == SIGINT || sig == SIGTERM) {
		sig_stop = sig;
	}
}

int main()
{
	int index_id;
	std::cout << "Please enter the ID of the index: " << std::endl;
	std::cin >> index_id;

	sig_stop = 0;
	std::signal(SIGINT, &check_signal);
	std::signal(SIGTERM, &check_signal);

	while (sig_stop == 0) {
		RunClient(index_id);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
