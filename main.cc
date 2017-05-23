#include <iostream>
#include "qp.hpp"

void TestQP001(void) {
   ReliableQueuePair<int> qp(2);
   for (int i = 0, a = 0, b = 0; i < 100; i++) {
      a = i;
      b = 0; 
      qp.SubmitRequest(a);
      qp.GetWorkRequest(a);
      qp.PostWorkReply(a);
      qp.GetWorkReply(b);
      std::cout << a << ":" << b << std::endl;
   }
}   

int main(void) {
   TestQP001();
   return 0;
}    
