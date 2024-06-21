#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <string>
//#include <sigpack.h>
//#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/

#include "../src/utils.h" // Assuming utils.h includes the definition of restartListener
#include "../src/custom_types.h"


TEST(ClearQueueTest, EmptyQueue) {
  // Create an empty queue
  std::queue<std::vector<uint8_t>> q;

  // Clear the queue
  ClearQueue(q);

  // Verify the queue is still empty
  EXPECT_TRUE(q.empty());
}

TEST(ClearQueueTest, NonEmptyQueue) {
  // Create a queue with some data
  std::queue<std::vector<uint8_t>> q;
  std::vector<uint8_t> data1 = {1, 2, 3};
  std::vector<uint8_t> data2 = {4, 5, 6};
  q.push(data1);
  q.push(data2);

  // Clear the queue
  ClearQueue(q);

  // Verify the queue is empty
  EXPECT_TRUE(q.empty());
}
