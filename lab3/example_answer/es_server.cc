/*
 * Copyright 2020 Chao Wang
 * This code is based on an example from the official gRPC GitHub
 * 
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "es.grpc.pb.h"

#include <unistd.h>
#include <sched.h>
#include <cmath>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::ServerReader;

using es::TopicRequest;
using es::EventService;
using es::TopicData;
using es::NoUse;

#define MAX_SUBSCRIBERS 10

// Logic and data behind the server's behavior.
class EventServiceImpl final : public EventService::Service {

 public:
  Status Subscribe(ServerContext* context,
                   const TopicRequest* request,
                   ServerWriter<TopicData>* writer) override {
    if (addSubscriber(request, writer)) {
      sleep(3600); // preserve the validity of the writer pointer
    }
    else {
      std::cerr << "error: subscription failed (MAX reached)" << std::endl;
    }
    return Status::OK;
  }

  Status Publish(ServerContext* context,
                   ServerReader<TopicData>* reader,
                   NoUse* nouse) override {
    TopicData td;
    while (reader->Read(&td)) {
      std::cout << "{" << td.topic() << ": " << td.data() << "}" << std::endl;
      writeToSubscribers(td);
    }
    return Status::OK;
  }

 private:

  bool addSubscriber(const TopicRequest* request,
                     ServerWriter<TopicData>* writer) {
    // Associate to the next available one in the list.
    bool success;
    if (subscriber_index <= MAX_SUBSCRIBERS) {
      subscribers_[subscriber_index] = writer;
      topics_[subscriber_index].set_topic(request->topic());
      subscriber_index++;
      success = true;
    }
    else {
      success = false;
    }
    return success;
  }

  void writeToSubscribers(TopicData td) {
    for (int i = 0; i < subscriber_index; i++) {
      if (topics_[i].topic() == td.topic()) {
        subscribers_[i]->Write(td);
      }
    }
  }

  ServerWriter<TopicData>* subscribers_[MAX_SUBSCRIBERS];
  TopicData topics_[MAX_SUBSCRIBERS]; //TODO: merge this to subscribers_
  int subscriber_index = 0; //FIXME: protect it by a mutex
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  EventServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
