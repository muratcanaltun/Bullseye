#include <iostream>
#include <string>
#include <csignal>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "bullseyestockexternal.grpc.pb.h"

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using bullseyestockexternal::StockInterface;
using bullseyestockexternal::SignUpRequest;
using bullseyestockexternal::SignUpReply;
using bullseyestockexternal::SignInRequest;
using bullseyestockexternal::SignInReply;
using bullseyestockexternal::SendOrderRequest;
using bullseyestockexternal::SendOrderReply;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

mongocxx::instance instance{};
mongocxx::uri uri("mongodb://localhost:27017");
mongocxx::client client(uri);
mongocxx::database db = client["Stock"];
mongocxx::collection orders = db["Orders"];
mongocxx::collection users = db["Users"];

class StockInterfaceServiceImplementation final : public StockInterface::Service
{

	Status signUp(ServerContext* context, const SignUpRequest* request, SignUpReply* reply) override
	{
		mongocxx::cursor cursor = users.find(
			document{} << "username" << request->user_name() << finalize);

		int username_count = 0;
		for (auto doc : cursor) {
			username_count++;
		}

		if (username_count > 0) {
			reply->set_reply("There is already a user with that username.");
		}
		else {
			auto builder = document{};

			bsoncxx::document::value doc_value = builder << "username" << request->user_name()
				<< "password" << request->password() << finalize;
			users.insert_one(doc_value.view());

			reply->set_reply("Sign up successful!");
		}

		return Status::OK;
	}

	Status signIn(ServerContext* context, const SignInRequest* request, SignInReply* reply) override
	{
		bsoncxx::stdx::optional<bsoncxx::document::value> user_data = users.find_one(document{} << "username" << request->user_name()
			<< "password" << request->password() << finalize);

		if (user_data.has_value()) {
			reply->set_reply("Sign in successful!");
		}
		else {
			reply->set_reply("Sign in failed!");
		}

		return Status::OK;
	}

	Status sendOrder(ServerContext* context, const SendOrderRequest* request, SendOrderReply* reply) override
	{
		auto builder = document{};

		bsoncxx::document::value doc_value = builder << "username" << request->user_name()
			<< "stock_id" << request->stock_id() 
			<< "stock_price" << request->stock_price()
			<< "order_type" << request->order_type() << finalize;

		orders.insert_one(doc_value.view());

		reply->set_reply("Order sent.");

		return Status::OK;
	}
};

void RunServer()
{
	std::string server_address("127.0.0.1:50057");
	StockInterfaceServiceImplementation service;

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Bullseye External Stock Interface Server listening on: " << server_address << std::endl;

	server->Wait();
}

int main()
{
	RunServer();
}