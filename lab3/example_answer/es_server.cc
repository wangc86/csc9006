/*
 * Copyright 2020 Chao Wang
 * This code is based on an example from the official gRPC GitHub
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

#include <queue>
#include <utility>
#include <chrono>
#include <fstream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using grpc::ServerWriter;
using grpc::ServerReader;

using es::TopicRequest;
using es::EventService;
using es::TopicData;
using es::NoUse;

bool nonEmptyPQ;
pthread_mutex_t mutex_PQ;
pthread_cond_t cv_PQ;

std::string* schedStrategy;
long long period[3];
int num[3];

#define MAX_SUBSCRIBERS 3

// The implementation of the event service.
class EventServiceImpl final : public EventService::Service {

 public:
  EventServiceImpl() {
    //pinCPU(0);
    // The following way to create a thread is derived from
    //  (1) https://thispointer.com/c-how-to-pass-class-member-function-to-pthread_create/
    //  (2) https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    // We need to do this in order to use a non-static member function
    // (making the member function static will cause other troubles..)
    typedef void * (*THREADFUNCPTR)(void *);
    pthread_create (&dispatching_threads[0], NULL,
                    (THREADFUNCPTR) &EventServiceImpl::dispatchTask,
                    this);
    // For now, we use only one dispatching thread;
    // it may be of interest to use multiple dispatching threads.
    //pinCPU(1);
    nonEmptyPQ = false;
    mutex_PQ = PTHREAD_MUTEX_INITIALIZER;
    cv_PQ = PTHREAD_COND_INITIALIZER;
    subscriber_index = 0;
    std::chrono::system_clock::time_point baseTime = std::chrono::system_clock::now();
  }

  Status Subscribe(ServerContext* context,
                   const TopicRequest* request,
                   ServerWriter<TopicData>* writer) override {
    if (addSubscriber(request, writer)) {
      sleep(36000); // preserve the validity of the writer pointer; sleep for 10 hours in this case
    }
    else {
      std::cerr << "error: subscription failed (MAX reached)" << std::endl;
      return Status(StatusCode::RESOURCE_EXHAUSTED, "Max. subscribers reached");
    }
    return Status::OK;
  }

  Status Publish(ServerContext* context,
                   ServerReader<TopicData>* reader,
                   NoUse* nouse) override {
    TopicData td;
    while (reader->Read(&td)) {
      std::cout << "{" << td.topic() << ": " << td.data() << "}" << std::endl;
      // writeToSubscribers(td);
      // Without de-coupling, we may invoke the above write function here;
      // with de-coupling, we will use another thread to invoke the function.
      // In our implementation, it will be the thread who takes data out of the
      // priority queue.
      pthread_mutex_lock(&mutex_PQ);
      int val = getVal(td);
      std::pair <int,TopicData> element (val, td);
      PQ.push(element);
      nonEmptyPQ = true;
      pthread_cond_broadcast(&cv_PQ);
      pthread_mutex_unlock(&mutex_PQ);
    }
    return Status::OK;
  }

 private:

  void* dispatchTask (void *) {
    std::cout << "Starting a dispatching task...\n";
    //setSchedulingPolicy(SCHED_FIFO, 99);
    while (1) {
      pthread_mutex_lock(&mutex_PQ);
      while (!nonEmptyPQ) {
        pthread_cond_wait(&cv_PQ, &mutex_PQ);
      }
      TopicData td = std::get<1>(PQ.top());
      PQ.pop();
      // TODO: check if indeed the PQ is empty
      nonEmptyPQ = false;
      pthread_mutex_unlock(&mutex_PQ);
      // Now, send the topic data to subscriber(s)
      writeToSubscribers(td);
    }
  }

  bool addSubscriber(const TopicRequest* request,
                     ServerWriter<TopicData>* writer) {
    // Associate to the next available one in the list.
    bool success;
    if (subscriber_index < MAX_SUBSCRIBERS) {
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
    // In the current implementation, we search for all subscribers.
    // It is possible to improve the performance by, for example,
    // tracking the set of interested subscribers. Though for a
    // small-scale application scenario it is not necessary.
    for (int i = 0; i < subscriber_index; i++) {
      if (topics_[i].topic() == td.topic()) {
        subscribers_[i]->Write(td);
      }
    }
  }

  int getVal(TopicData td) {
    long long val;
    if (*schedStrategy == "EDF") {
      val = 1;
    }
    else if (*schedStrategy == "RM") {
      val = period[(td.topic()).front() - '0'];
    }
    else if (*schedStrategy == "FIFO") {
      std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
      val = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - baseTime).count();
    }
    else {
      std::cerr << "Error: undefined scheduling strategy\n";
      exit(1);
    }
    return val;
  }

  // An auxiliary class that implements the priority-queue's comparing function
  // In this version, the element with the smallest value will be popped first.
  // Therefore, for EDF, we push into this queue the deadline value;
  // for RM, we push the period value;
  // for FIFO, we push the timestamp value.
  // We should improve this implementation to make the strategy transparent.
  class myComparison {
  public:
    myComparison() {};
    bool operator() (const std::pair<int,TopicData>& lhs, const std::pair<int,TopicData>& rhs) const
    {
      if (std::get<0>(lhs) > std::get<0>(rhs))
        return true;
      else
        return false;
    }
  };

  ServerWriter<TopicData>* subscribers_[MAX_SUBSCRIBERS];
  TopicData topics_[MAX_SUBSCRIBERS]; //TODO: merge this to subscribers_
  int subscriber_index; //FIXME: protect it by a mutex
  std::priority_queue< std::pair<int,TopicData>,
                       std::vector<std::pair<int,TopicData>>,
                       myComparison > PQ;
  pthread_t dispatching_threads[1];
  std::chrono::system_clock::time_point baseTime;
};

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cout << "Usage: " << argv[0] << " -c configurationFile -s [EDF|RM|FIFO]\n";
    exit(1);
  }

  std::string str(argv[4]);
  if (str != "EDF" && str != "RM" && str != "FIFO") {
    std::cerr << "Error: undefined scheduling strategy\n";
    exit(1);
  }
  schedStrategy = &str;

  // Reading the configuration file of the following format:
  // topicRate #ofSuchTopics
  // topicRate #ofSuchTopics
  // topicRate #ofSuchTopics
  // where topic rate is in terms of a time interval in microseconds
  std::ifstream fi;
  fi.open(argv[2]);
  for (int i = 0; i < 3; i++) {
    fi >> period[i];
    fi >> num[i];
  }
  fi.close();

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


  return 0;
}
