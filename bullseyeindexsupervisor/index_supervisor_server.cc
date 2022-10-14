#include <string>

#include <grpcpp/grpcpp.h>
#include "bullseyeindexsupervisor.grpc.pb.h"
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <rapidjson/document.h>

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

using bullseyeindexsupervisor::IndexReply;
using bullseyeindexsupervisor::IndexRequest;
using bullseyeindexsupervisor::IndexCheck;

mongocxx::instance instance{};
mongocxx::client client{ mongocxx::uri{} };
mongocxx::database db = client["Index"];
mongocxx::collection coll = db["Index_Values"];

class IndexSupervisorServiceImplementation final : public IndexCheck::Service
{

	Status sendRequest(ServerContext* context, const IndexRequest* request, IndexReply* reply) override
	{
		bsoncxx::stdx::optional<bsoncxx::document::value> index_data =
			coll.find_one(document{} << "Name" << request->index_name() << finalize);

		std::string index_json = bsoncxx::to_json(*index_data);

		rapidjson::Document doc;
		doc.Parse<0>(index_json.c_str());

		rapidjson::Value& index_value = doc["Value"];
		std::string val_str = index_value.GetString();

		reply->set_index_value(std::atof(val_str.c_str()));
		return Status::OK;
	}
};

void RunServer()
{
	std::string server_address("127.0.0.1:50055");
	IndexSupervisorServiceImplementation service;

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Bullseye Index Supervisor listening on: " << server_address << std::endl;

	server->Wait();
}

int main()
{
	RunServer();
}