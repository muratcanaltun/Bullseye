#include <grpcpp/grpcpp.h>
#include <string>
#include "bullseyeindexservice.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using bullseyeindexservice::IndexReply;
using bullseyeindexservice::IndexRequest;
using bullseyeindexservice::IndexCalc;

class IndexCalcServiceImplementation final : public IndexCalc::Service
{
	Status sendRequest(ServerContext* context, const IndexRequest* request, IndexReply* reply) override
	{
		int index_id = request->index_id();

		int n = 1;
		int price = 0;

		switch (index_id)
		{
		case 1:
			n = 10;
			break;
		case 2:
			n = 100;
			break;
		default:
			n = 10000;
			break;
		}

		for (int i = 1; i <= n; i++) {
			price += i;
		}

		// randomized the price to view differences in the output
		price += std::rand();

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

int main()
{
	RunServer();
}
