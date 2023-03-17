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

using grpc::Channel;
using grpc::ClientContext;
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

bool signed_in = false;
std::string current_user;

namespace
{
	volatile std::sig_atomic_t sig_stop;
}

class User_Data {
public:
	std::string user_name;
	std::string password;
};

class Order_Data {
public:
	std::string user_name;
	std::string stock_id;
	double stock_price;
	int order_type;
};

class StockInterfaceClient {
public:
	StockInterfaceClient(std::shared_ptr<Channel> channel) : stub_(StockInterface::NewStub(channel)) {}

	std::string signUp(User_Data user_data)
	{
		SignUpRequest request;

		request.set_user_name(user_data.user_name);
		request.set_password(user_data.user_name);

		SignUpReply reply;
		ClientContext context;

		Status status = stub_->signUp(&context, request, &reply);

		if (status.ok())
		{
			return reply.reply();
		}
		else
		{
			std::cout << "Error code " << status.error_code() << ": " << status.error_message() << std::endl;
			return "RPC Failed";
		}
	}

	std::string signIn(User_Data user_data)
	{
		SignInRequest request;

		request.set_user_name(user_data.user_name);
		request.set_password(user_data.user_name);

		SignInReply reply;
		ClientContext context;

		Status status = stub_->signIn(&context, request, &reply);

		if (status.ok())
		{
			return reply.reply();
		}
		else
		{
			std::cout << "Error code " << status.error_code() << ": " << status.error_message() << std::endl;
			return "RPC Failed";
		}
	}

	std::string sendOrder(Order_Data order_data)
	{
		SendOrderRequest request;

		SendOrderReply reply;
		ClientContext context;

		request.set_user_name(order_data.user_name);
		request.set_stock_id(order_data.stock_id);
		request.set_stock_price(order_data.stock_price);
		request.set_order_type(order_data.order_type);

		Status status = stub_->sendOrder(&context, request, &reply);

		if (status.ok())
		{
			return reply.reply();
		}
		else
		{
			std::cout << "Error code " << status.error_code() << ": " << status.error_message() << std::endl;
			return "RPC Failed";
		}
	}

private:
	std::unique_ptr<StockInterface::Stub> stub_;
};

void RunClient()
{
	// init IP adress, response str and client
	std::string target_address("127.0.0.1:50057");
	std::string response;
	StockInterfaceClient client(grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials()));

	int option;

	if (!signed_in) {
		User_Data user_data;
		std::cout << "Please choose an option:\n1. Sign In\n2. Sign Up" << std::endl;
		std::cin >> option;

		std::cout << "Please enter your user name:" << std::endl;
		std::cin >> user_data.user_name;

		std::cout << "Please enter your password:" << std::endl;
		std::cin >> user_data.password;
		
		switch (option)
		{
		case 1:
			response = client.signIn(user_data);
			
			if (response.compare("Sign in successful!") == 0) {
				signed_in = true;
				current_user = user_data.user_name;
			}

			break;
		case 2:
			response = client.signUp(user_data);
			break;
		}

		option = 0;
	}
	else {
		Order_Data order_data;
		std::cout << "Please choose an option:\n1. Send Order\n2. Sign Out" << std::endl;
		std::cin >> option;

		switch (option)
		{
		case 1:
			order_data.user_name = current_user;
			std::cout << "Please enter the ID of the stock you want to work with: (e.g. AA)" << std::endl;
			std::cin >> order_data.stock_id;

			std::cout << "Please enter the price you want for it:" << std::endl;
			std::cin >> order_data.stock_price;

			std::cout << "Please choose an option:\n1. Buy\n2. Sell" << std::endl;
			std::cin >> order_data.order_type;

			response = client.sendOrder(order_data);
			std::cout << response << std::endl;
			break;
		case 2:
			signed_in = false;
			current_user.clear();

			std::cout << "Logging out...\n" << std::endl;
			break;
		}

		option = 0;
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
	std::cout << "Welcome to Bullseye External Stock Interface." << std::endl;
	while (sig_stop == 0) {
		RunClient();
	}
}