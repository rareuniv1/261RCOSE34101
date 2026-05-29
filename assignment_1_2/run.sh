#!/bin/sh

gcc message_buffer_semaphore.c consumer.c -o consumer -pthread

gcc message_buffer_semaphore.c producer.c -o producer -pthread

gcc destroy.c -o destroy -pthread
