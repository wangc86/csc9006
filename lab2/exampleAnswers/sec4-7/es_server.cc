/*
 * 
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

pthread_mutex_t mutexHigh;
pthread_mutex_t mutexMid;
pthread_mutex_t mutexLow;
pthread_cond_t cvHigh;
pthread_cond_t cvMid;
pthread_cond_t cvLow;
bool hasDataHigh, hasDataMid, hasDataLow;
TopicData tdHigh, tdMid, tdLow;
ServerWriter<TopicData>* writerHigh = NULL;
ServerWriter<TopicData>* writerMid = NULL;
ServerWriter<TopicData>* writerLow = NULL;

void pinCPU (int cpu_number)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(cpu_number, &mask);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1)
    {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

void setSchedulingPolicy (int newPolicy, int priority)
{
    sched_param sched;
    int oldPolicy;
    if (pthread_getschedparam(pthread_self(), &oldPolicy, &sched)) {
        perror("pthread_setschedparam");
        exit(EXIT_FAILURE);
    }
    sched.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), newPolicy, &sched)) {
        perror("pthread_setschedparam");
        exit(EXIT_FAILURE);
    }
}

void workload_1ms ()
{
    double c = 10.1;
    int repeat = 70000;
    for (int i = 1; i <= repeat; i++)
    {
        c = sqrt(i*i*c);
    }
}

void* highPriorityTask (void *id) {
  std::cout << "Starting the high-priority task...\n";
  setSchedulingPolicy(SCHED_FIFO, 99);
  while (1) {
    pthread_mutex_lock(&mutexHigh);
    while (!hasDataHigh) {
      pthread_cond_wait(&cvHigh, &mutexHigh);
    }
    if (writerHigh != NULL) {
      workload_1ms ();
      std::cout << "{" << tdHigh.topic() << ": " << tdHigh.data() << "}" << std::endl;
      writerHigh->Write(tdHigh);
    }
    else {
      std::cout << "no subscriber; discard the data\n";
    }
    hasDataHigh = false;
    pthread_mutex_unlock(&mutexHigh);
  }
}

void* midPriorityTask (void *id) {
  std::cout << "Starting the middle-priority task...\n";
  setSchedulingPolicy(SCHED_FIFO, 98);
  while (1) {
    pthread_mutex_lock(&mutexMid);
    while (!hasDataMid) {
      pthread_cond_wait(&cvMid, &mutexMid);
    }
    if (writerMid != NULL) {
      for (int i = 0; i < 10; i++) {
        workload_1ms ();
      }
      std::cout << "{" << tdMid.topic() << ": " << tdMid.data() << "}" << std::endl;
      writerMid->Write(tdMid);
    }
    else {
      std::cout << "no subscriber; discard the data\n";
    }
    hasDataMid = false;
    pthread_mutex_unlock(&mutexMid);
  }
}

void* lowPriorityTask (void *id) {
  std::cout << "Starting the low-priority task...\n";
  setSchedulingPolicy(SCHED_FIFO, 97);
  while (1) {
    pthread_mutex_lock(&mutexLow);
    while (!hasDataLow) {
      pthread_cond_wait(&cvLow, &mutexLow);
    }
    if (writerLow != NULL) {
      for (int i = 0; i < 50; i++) {
        workload_1ms ();
      }
      std::cout << "{" << tdLow.topic() << ": " << tdLow.data() << "}" << std::endl;
      writerLow->Write(tdLow);
    }
    else {
      std::cout << "no subscriber; discard the data\n";
    }
    hasDataLow = false;
    pthread_mutex_unlock(&mutexLow);
  }
}


// Logic and data behind the server's behavior.
class EventServiceImpl final : public EventService::Service {

 public:
  EventServiceImpl() {
    pinCPU(0);
    pthread_create (&edgeComputing_threads[0], NULL,
                     highPriorityTask, (void *) &idp[0]);
    pthread_create (&edgeComputing_threads[1], NULL,
                     midPriorityTask, (void *) &idp[1]);
    pthread_create (&edgeComputing_threads[2], NULL,
                     lowPriorityTask, (void *) &idp[2]);
    pinCPU(1);
    hasDataHigh = false;
    hasDataMid = false;
    hasDataLow = false;
    mutexHigh = PTHREAD_MUTEX_INITIALIZER;
    mutexMid = PTHREAD_MUTEX_INITIALIZER;
    mutexLow = PTHREAD_MUTEX_INITIALIZER;
    cvHigh = PTHREAD_COND_INITIALIZER;
    cvMid = PTHREAD_COND_INITIALIZER;
    cvLow = PTHREAD_COND_INITIALIZER;
  }

  Status Subscribe(ServerContext* context,
                   const TopicRequest* request,
                   ServerWriter<TopicData>* writer) override {
  //TODO: use request to determine topic subscription
    writerHigh = writer;
    writerMid = writer;
    writerLow = writer;
    sleep(3600); // preserve the validity of the writer pointer
    return Status::OK;
  }

  Status Publish(ServerContext* context,
                   ServerReader<TopicData>* reader,
                   NoUse* nouse) override {
    TopicData td;
    while (reader->Read(&td)) {
      if (td.topic() == "High") {
        pthread_mutex_lock(&mutexHigh);
        tdHigh = td;
        hasDataHigh = true;
        pthread_cond_broadcast(&cvHigh);
        pthread_mutex_unlock(&mutexHigh);
      } else if (td.topic() == "Middle") {
        pthread_mutex_lock(&mutexMid);
        tdMid = td;
        hasDataMid = true;
        pthread_cond_broadcast(&cvMid);
        pthread_mutex_unlock(&mutexMid);
      } else if (td.topic() == "Low") {
        pthread_mutex_lock(&mutexLow);
        tdLow = td;
        hasDataLow = true;
        pthread_cond_broadcast(&cvLow);
        pthread_mutex_unlock(&mutexLow);
      } else {
        std::cerr << "Publish: got an unidentified topic!\n";
      }
    }
    return Status::OK;
  }

 private:
  ServerWriter<TopicData>* writer_ = NULL;
  pthread_t edgeComputing_threads[3];
  const int idp[3] = {0, 1, 2}; // id of each pthread
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
