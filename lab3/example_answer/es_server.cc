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
#include <sys/time.h>

#include <queue>
#include <utility>
#include <chrono>
#include <fstream>

#include <google/protobuf/util/time_util.h>

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
using google::protobuf::Timestamp;

bool nonEmptyPQ;
pthread_mutex_t mutex_PQ;
pthread_cond_t cv_PQ;

std::string* schedStrategy;
long long period[3];
int num[3];

#define MAX_SUBSCRIBERS 3

void pinCPU (int cpu_number)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(cpu_number, &mask);

    // Note: for pthreads, use pthread_setaffinity_np instead
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1)
    {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

void setSchedulingPolicy (int newPolicy, int priority)
{
    sched_param sched;
    if (sched_getparam(0, &sched)) {
        perror("sched_getparam");
        exit(EXIT_FAILURE);
    }
    sched.sched_priority = priority;
    if (sched_setscheduler(0, newPolicy, &sched)) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }
}

void setSchedulingPolicyThread (int newPolicy, int priority)
{
    sched_param sched;
    int oldPolicy;
    if (pthread_getschedparam(pthread_self(), &oldPolicy, &sched)) {
        perror("pthread_getschedparam");
        exit(EXIT_FAILURE);
    }
    sched.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), newPolicy, &sched)) {
        perror("pthread_setschedparam");
        exit(EXIT_FAILURE);
    }
}

// The implementation of the event service.
class EventServiceImpl final : public EventService::Service {

 public:
  EventServiceImpl() {
    pinCPU(0);
    setSchedulingPolicy(SCHED_FIFO, 98);
    // The following way to create a thread is derived from
    //  (1) https://thispointer.com/c-how-to-pass-class-member-function-to-pthread_create/
    //  (2) https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    // We need to do this in order to use a non-static member function
    // (making the member function static will cause other troubles.).
    // No need to do this if we were to move the dispatchTask outside this class,
    // but then we should not have the dispatchTask access private members in the class..
    // Object-Oriented Design is an art that takes much experience to master.
    typedef void * (*THREADFUNCPTR)(void *);
    pthread_create (&dispatching_threads[0], NULL,
                    (THREADFUNCPTR) &EventServiceImpl::dispatchTask,
                    this);
    // For now, we use only one dispatching thread;
    // As a further study, try to use multiple dispatching threads.
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
      sleep(36000); // preserve the validity of the writer pointer (sleep for 10 hours in this case)
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
    struct timeval tv;
    while (reader->Read(&td)) {
      // Often, we need to remove debug messages before shipping our program, because
      // outputing debug messages to IO may severely impact the timing performance of our program.
      //std::cout << "{" << td.topic() << ": " << td.data() << "}  ";// << std::endl;

      //gettimeofday(&tv, NULL);
      //std::cout << "response time = " << (tv.tv_sec - td.timestamp().seconds())*1000000 + (tv.tv_usec*1000 - td.timestamp().nanos())/1000 << "us\n";
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
    //std::cout << "Starting a dispatching task...\n";
    //pinCPU(1);
    setSchedulingPolicyThread(SCHED_FIFO, 99);
    while (1) {
      pthread_mutex_lock(&mutex_PQ);
      while (!nonEmptyPQ) {
        pthread_cond_wait(&cv_PQ, &mutex_PQ);
      }
      TopicData td = std::get<1>(PQ.top());
      PQ.pop();
      // check if indeed the PQ is empty
      if (PQ.size() == 0) {
        nonEmptyPQ = false;
      }
      // As an exercise, comment out the above condition
      // and see how that would change the end-to-end latency.
      // How would you explain this phenomenon?
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
      if ((topics_[i].topic().front()-'0') == (td.topic()).front()-'0') {
        subscribers_[i]->Write(td);
      }
    }
  }

  int getVal(TopicData td) {
    long long val;
    //long long val_faultTolerant;
    if (*schedStrategy == "EDF") {
      // Here we suppose the implicit deadline.
      // The absolute deadline is defined to be
      // the relative deadline plus the timepoint of message creation,
      // where for the case of implcit deadline the relative
      // deadline is equal to the topic period.
      // And to simply the design, we assume that it takes equal time to successfully
      // deliver each message to the subscriber.
      // Also, see the comment for the RM scheduing strategy below..
      val = period[(td.topic()).front() - '0'] + google::protobuf::util::TimeUtil::TimestampToMicroseconds(td.timestamp());
      // Now, suppose that the publisher can retain k lastest message,
      // i.e., N=k. Then the absolute dispatching deadline would be the earliest
      // among the value calculated above and that calculated below:
      //  val_faultTolerant = period[(td.topic()).front() - '0'] + google::protobuf::util::TimeUtil::TimestampToMicroSeconds(td.timestamp());
      // It is clear that as long as we suppose the implicit deadline,
      // val will be no larger than val_faultTolerant (even if N=1).
      // Therefore, we simple use the value of val calculated above.
    }
    else if (*schedStrategy == "RM") {
      val = period[(td.topic()).front() - '0'];
      // The above is an example of bad programming practice,
      // because it tangled the following:
      //  (a) the index to the topic period, and
      //  (b) the encoding of the topic name.
      // With a good design, (a) and (b) should be independent.
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
  // Therefore, for EDF, we push into this queue the absolute deadline value;
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
  //std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();


  return 0;
}
