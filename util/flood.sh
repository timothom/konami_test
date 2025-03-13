#!/bin/bash
#Stress test by running lots of clients

program_to_run="./build/client ./messages/xml-message.txt"

for i in {1..100000}; do
  $program_to_run &
done
