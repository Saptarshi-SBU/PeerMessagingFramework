/*-----------------------------------------------------------------------
 * Strand Based Executor based on Boost IO Service
 *
 * Refs:https://stackoverflow.com :useboost-condition-variable-to-wait
 * Refs: missed wakeups
 *
 * ----------------------------------------------------------------------*/
#ifndef _WORKER_HPP__
#define _WORKER_HPP__

#include "chrono.hpp"

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/atomic.hpp>

class SharedIntrusive {
protected:
   SharedIntrusive():
      useCount(0)
   {}

   virtual ~SharedIntrusive() {}

   friend inline void intrusive_ptr_release(const SharedIntrusive *obj)
   {
      if (__sync_fetch_and_sub(&(obj->useCount), 1) == 1) {
         delete obj;
      }
    }
   
   friend inline void intrusive_ptr_add_ref(const SharedIntrusive *obj)
   {
      __sync_fetch_and_add(&(obj->useCount), 1);
   }

public:
   virtual void DumpStats() {}
   inline int GetUseCount(void)
   {
      return useCount;
   }

private:
   mutable int useCount;
};

/* Completion Token for notification  */
class CompletionToken: public SharedIntrusive {
   public:
   typedef boost::intrusive_ptr<CompletionToken> pointer;

   CompletionToken():
       done(false),
       cond(),
       mutex(),
       tp(Timer::Now()) {}

   void Wait() {
      boost::unique_lock<boost::mutex> nanny(mutex);
      while (!done) {
         cond.wait(nanny);
      }
   }

   virtual void Signal() {
      boost::unique_lock<boost::mutex> nanny(mutex);
      done = true;
      cond.notify_one();
   }

   bool done;
   boost::condition_variable cond;
   Timer::TimePoint tp;

   private:
   boost::mutex mutex;
};

class Service : public SharedIntrusive {

   public:
   typedef boost::intrusive_ptr<Service> pointer;

   Service(std::string workerName, int nr_threads) :
      name(workerName),
      work_(io_svc_) {
      CreateThreadPool(nr_threads);
   }

   void Stop() {
      io_svc_.stop();
      try {
         threadpool_.join_all();
      } catch (std::exception&) {}
   }

   virtual ~Service() {
      std::cout << "~Service" << std::endl; 
      Stop();
   }

   // Threads on which io_service runs
   void CreateThreadPool(int nr_threads) {
      for (int i = 0; i < nr_threads; i++) {
          boost::thread *t = threadpool_.create_thread
              (boost::bind(&boost::asio::io_service::run, &io_svc_));
          std::cout << "Launced thread : " << t->get_id() << std::endl;
      }
   }

   boost::asio::io_service& GetIOService(void) {
      return io_svc_;
   }

   private:

   std::string name;
   boost::asio::io_service io_svc_;
   boost::thread_group threadpool_;
   boost::asio::io_service::work work_;
};

class Executor : public SharedIntrusive {

    public:
    Executor() {}
    virtual ~Executor() {}  // cannot be 'pure'; destructor invoked in reverse order
};

class StrandExecutor : public Executor {
   public:
   typedef boost::intrusive_ptr<StrandExecutor> pointer;
   StrandExecutor(std::string strandName, boost::asio::io_service &io_svc) :
      Executor(), 
      name(strandName), 
      strand(io_svc)
    {}

   ~StrandExecutor() {}

   virtual void Submit(const boost::function<void()>& f) {
      if (strand.running_in_this_thread()) {
         strand.dispatch(f);
      } else {
         strand.post(f);
      }
   }

   private:
   std::string name;
   boost::asio::strand strand;
};
#endif
