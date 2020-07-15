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
#include <fstream>
#include <tuple>
#include <chrono>
#include <unistd.h>

#include <grpcpp/grpcpp.h>

#include "es.grpc.pb.h"
#include <sys/time.h>
#include <sched.h>

#include <google/protobuf/util/time_util.h>


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;

using es::TopicRequest;
using es::EventService;
using es::TopicData;
using es::NoUse;

using google::protobuf::Timestamp;

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

class Publisher {
 public:
  Publisher(int pubID, std::shared_ptr<Channel> channel)
      : id_(pubID),
        stub_(EventService::NewStub(channel)), 
        writer_(stub_->Publish(&context_, &nouse_)) {}

  void Publish(const TopicData td) {
    if (!writer_->Write(td)) {
      // Broken stream.
      std::cout << "rpc failed: broken stream." << std::endl;
      writer_->WritesDone();
      Status status = writer_->Finish();
      if (status.ok()) {
        std::cout << "Finished.\n";
      } else {
        std::cout << "RecordRoute rpc failed." << std::endl;
      }
      exit(0);
    }
    else {
      std::cout << "sent {" << td.topic() << ": " << td.data() << "}" << std::endl;
    }
  }

  void done() {
    writer_->WritesDone();
    Status status = writer_->Finish();
    if (status.ok()) {
      std::cout << "Finished.\n";
    } else {
      std::cout << "RecordRoute rpc failed." << std::endl;
    }
  }

 private:
  std::unique_ptr<EventService::Stub> stub_;
  int id_;
  ClientContext context_;
  NoUse nouse_;
  std::unique_ptr<ClientWriter<TopicData> > writer_;
};

void* pubTask (void* arg) {
  std::string target_str = "localhost:50051";
  Publisher pub(1, grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));
  std::tuple<std::string,int> tup = * (std::tuple<std::string,int>*) arg;
  std::string topic;
  long long period;
  std::tie (topic, period) = tup;
  std::cout << topic << " " << period << std::endl;

  TopicData td;
  td.set_topic(topic);
  td.set_data("test");

  // send data periodically
  struct timeval tv;
  int delta;
  while(1) {
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    Timestamp *tp = td.mutable_timestamp();
    //Timestamp timestamp = google::protobuf::util::TimeUtil::GetCurrentTime(); // It seems that its resolution is only up to seconds
    //tp->set_seconds(timestamp.seconds());
    //tp->set_nanos(timestamp.nanos());
    gettimeofday(&tv, NULL);
    tp->set_seconds(tv.tv_sec);
    tp->set_nanos(tv.tv_usec*1000);
    pub.Publish(td);
    std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
    delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    if (delta > period) {
      continue;
    }
    else {
      usleep (period-delta);
    }
  }
  pub.done();
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " -c configurationFile\n";
    exit(1);
  }

  // Reading the configuration file of the following format:
  // topicRate #ofSuchTopics
  // topicRate #ofSuchTopics
  // topicRate #ofSuchTopics
  // where topic rate is in terms of a time interval in microseconds
  int period[3];
  int num[3];
  std::ifstream fi;
  fi.open(argv[2]);
  for (int i = 0; i < 3; i++) {
    fi >> period[i];
    fi >> num[i];
  }
  fi.close();

  pthread_t** pub_threads = new pthread_t*[3];
  pub_threads[0] = new pthread_t[num[0]];
  pub_threads[1] = new pthread_t[num[1]];
  pub_threads[2] = new pthread_t[num[2]];
  pinCPU(3);
  setSchedulingPolicy(SCHED_RR, 99);
  for (int i = 0; i < 3; i++) {
    std::string istring = std::to_string(i);
    for (int j = 0; j < num[i]; j++) {
      std::string jstring = std::to_string(j);
      std::string ij = istring+jstring;
      std::tuple<std::string,int> arg(ij,period[i]);
      pthread_create (&pub_threads[i][j], NULL, pubTask, (void *) &arg);
      sleep(1);
    }
  }

  sleep(36000);

  return 0;
}
