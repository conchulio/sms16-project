#!/bin/bash

g++ sms-main.cc \
  sms-helpers.cc \
  sms-echo-client.cc \
  sms-echo-helper.cc \
  -o sms-main \
  -pthread -DNS3_OPENMPI -DNS3_MPI -pthread -I/usr/include/ns3.17 -I/usr/lib/openmpi/include -I/usr/lib/openmpi/include/openmpi -I/usr/include/ns3.17 -L/usr//lib -L/usr/lib/openmpi/lib -lns3.17-wifi -lm -lns3.17-propagation -lns3.17-mobility -lns3.17-tools -lns3.17-stats -lns3.17-internet -lns3.17-bridge -lns3.17-mpi -pthread -lmpi_cxx -lmpi -ldl -lhwloc -lns3.17-network -lns3.17-core -lrt -lm
