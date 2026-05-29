#!/bin/sh

gcc message_buffer.c consumer.c -o consumer

gcc message_buffer.c producer.c -o producer

gcc message_buffer.c destroy.c -o destroy
