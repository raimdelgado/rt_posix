 /*
  *  This file is owned by the Embedded Systems Laboratory of Seoul National University of Science and Technology
  *  as a part of RT-AIDE or the RTOS-Agnostic and Interoperable Development Environment for Real-time Systems
  *  
  *  File: TestRTPosix.cpp
  *  Author: 2022 Raimarius Delgado
  *  Description: Unit test for the RT-Posix Library based on GTest
  * 
  * 
  * 
  * 
 */
 #include "UnitTest.h"
 #include "TestRTPosix.cpp"

 int main(int argc, char **argv) 
 {
     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
 }