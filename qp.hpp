/*-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2017 Saptarshi Sen
 *
 *  QueuePair Interface For AsyncIO
 *
 * ---------------------------------------------------------------------------*/
#include <queue>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#ifndef _QP_H
#define _QP_H

template<class T>
class QueuePair {

    protected:
        std::queue<T> submission, completion;

    public:
        QueuePair() {}
        virtual ~QueuePair() {}

        virtual void SubmitRequest(const T& p) = 0;
        virtual void GetReply(T& p) = 0;
};

template<class T>
class BoundedQueuePair : public QueuePair<T> {

    protected:
        inline bool isNotFull(std::queue<T>& p) const {
            return p.size() < nr;
        }

        inline bool isNotEmpty(std::queue<T>& p) const {
            return !p.empty();
        }

        inline void GetSize(std::queue<T>& p) const {
            return p.size();
        }

        size_t nr;
    public:
        BoundedQueuePair(size_t n) : nr(n) {}
        virtual ~BoundedQueuePair() {}
};

template<class T>
class ReliableQueuePair : public BoundedQueuePair<T> {

     public:
         virtual void PostWorkRequest(const T& p) {
             boost::shared_lock<boost::shared_mutex> nanny(submtx);

             subFull.wait(nanny,
                boost::bind(&ReliableQueuePair::isNotFull, this, 
                ReliableQueuePair<T>::submission));
             ReliableQueuePair<T>::submission.push(p);
         }    

         virtual void GetWorkRequest(T& p) {
             boost::shared_lock<boost::shared_mutex> nanny(submtx);

             subEmpty.wait(nanny,
                boost::bind(&ReliableQueuePair::isNotEmpty, this,
                ReliableQueuePair<T>::submission));
             p = ReliableQueuePair<T>::submission.front();
             ReliableQueuePair<T>::submission.pop();
         }    

         virtual void PostWorkReply(const T& p) {
             boost::shared_lock<boost::shared_mutex> nanny(cplmtx);

             cplFull.wait(nanny,
                boost::bind(&ReliableQueuePair::isNotFull, this,
                ReliableQueuePair<T>::completion));
             ReliableQueuePair<T>::completion.push(p);
         }    

         virtual void GetWorkReply(T& p) {
             boost::shared_lock<boost::shared_mutex> nanny(cplmtx);

             cplEmpty.wait(nanny,
                boost::bind(&ReliableQueuePair::isNotEmpty, this,
                ReliableQueuePair<T>::completion));
             p = ReliableQueuePair<T>::completion.front();
             ReliableQueuePair<T>::completion.pop();
         }

     public:

         ReliableQueuePair(size_t n) : BoundedQueuePair<T>(n) {}

         virtual ~ReliableQueuePair() {}

         virtual void SubmitRequest(const T& p) {
             PostWorkRequest(p);
         }

         virtual void GetReply(T& p) {
             GetWorkReply(p);
         }

     private:
         mutable boost::shared_mutex submtx, cplmtx;
         mutable boost::condition subFull, subEmpty, cplFull, cplEmpty;
};
#endif
