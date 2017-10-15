/*----------------------------------------------------------------------------
 *
 * Copyright (C) 2017 Saptarshi Sen
 *
 * Benchmarking Strand Executors
 *
 * g++ -std=c++11 main.cc -lboost_thread -lboost_timer -lboost_chrono -g
 *
 * --------------------------------------------------------------------------*/
#include "chrono.hpp"
#include "worker.hpp"

#include <iostream>
#include <cassert>

#define HANDLER_LATENCY 1
#define WORKLOAD_THREADS 1

class WorkLoad {

    public:
    WorkLoad(StrandExecutor::pointer executor, unsigned int thresholdLatency) :
      executor_(executor),  
      stopped_(false),
      thresholdLatency_(thresholdLatency) {}

   ~WorkLoad() {}

    void Start(void) {
       for (int i = 0; i < WORKLOAD_THREADS; i++) {
          threadpool_.create_thread(boost::bind(&WorkLoad::Work, this));
       }
    }

    void Stop(void) {
      stopped_ = true;  
      try {
         threadpool_.join_all();
      } catch (std::exception&) {}
    }    

    void Work(void) {
       CompletionToken::pointer token;
       token = new CompletionToken();
       while (!stopped_) {  
          token->tp = Timer::Now(); 
          // Fix : was causing negative time
          token->done = false;
          executor_->Submit(boost::bind(&WorkLoad::WorkHandler, this, token));
          token->Wait(); 
       }
    }

    void WorkHandler(CompletionToken::pointer token) {
      uint64_t nsecs = 0;
      auto tp = Timer::Now();
      assert (tp >= token->tp);
      nsecs = Timer::NanoSecsDelta(token->tp, tp);
      if (nsecs > thresholdLatency_) {
         std::cout << "OpLatencywarning : " << nsecs << std::endl;
      }    
      token->Signal();
   }

   private:
   bool stopped_; 
   boost::thread_group threadpool_;
   StrandExecutor::pointer executor_;
   unsigned int thresholdLatency_;
};   

int main(void) {
   Service::pointer svc = new Service("test_worker", 10);
   StrandExecutor::pointer executor = new StrandExecutor("test_strand", svc->GetIOService());
   WorkLoad workLoad(executor, HANDLER_LATENCY);
   workLoad.Start();
   boost::this_thread::sleep(boost::posix_time::milliseconds(10000));
   workLoad.Stop();
   svc->Stop();
   return 0;
}    
