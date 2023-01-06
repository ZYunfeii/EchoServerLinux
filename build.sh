#!/bin/sh

g++ -g client.cpp -o client & g++ -g server_epoll.cpp -o server_epoll \
& g++ -g server_poll.cpp -o server_poll & g++ -g server_select.cpp -o server_select;

chmod 777 server_epoll;
chmod 777 client;
chmod 777 server_poll;
chmod 777 server_select;