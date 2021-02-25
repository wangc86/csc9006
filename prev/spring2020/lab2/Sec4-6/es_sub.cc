/*
 * Copyright 2020 Chao Wang
 * This code is based on an example from the official gRPC GitHub
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

#include "es.grpc.pb.h"

#include <unistd.h>
#include <sys/time.h>

#include <google/protobuf/util/time_util.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using es::TopicRequest;
using es::EventService;
using es::TopicData;
using es::NoUse;

using google::protobuf::Timestamp;

class Subscriber {
 public:
  Subscriber(int pubID, std::shared_ptr<Channel> channel)
      : id_(pubID),
        stub_(EventService::NewStub(channel)) {}

  void Subscribe(TopicRequest request) {
    ClientContext context;
    TopicData td;
    struct timeval tv;
    std::unique_ptr<ClientReader<TopicData> > reader(
        stub_->Subscribe(&context, request));
    std::cout << "Start to receive data...\n";
    while (reader->Read(&td)) {
//      Timestamp timestamp = google::protobuf::util::TimeUtil::GetCurrentTime();
      gettimeofday(&tv, NULL);
      std::cout << "{" << td.topic() << ": " << td.data() << "}  ";
      std::cout << "response time = " << tv.tv_sec - td.timestamp().seconds() << "s " << tv.tv_usec - td.timestamp().nanos()/1000 << "us\n";
      //std::cout << "response time = " << google::protobuf::util::TimeUtil::TimestampToMicroseconds(timestamp) - google::protobuf::util::TimeUtil::TimestampToMicroseconds(td.timestamp()) << " ms\n";
    }
    Status status = reader->Finish();
  }

 private:
  std::unique_ptr<EventService::Stub> stub_;
  int id_;
};

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " -t topic\n";
    exit(0);
  }
  std::string target_str;
  target_str = "localhost:50051";
  Subscriber sub(2, grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));
  TopicRequest request;
  request.set_topic(argv[2]);
  sub.Subscribe(request);
  sleep(3600);

  return 0;
}
