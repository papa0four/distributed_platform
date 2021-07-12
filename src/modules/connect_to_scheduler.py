#!/usr/bin/env python3

import os
import socket

if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "connect_to_scheduler"

import modules.broadcast as bc

def connect_to_scheduler() -> None:
    serv_info = bc.broadcast_and_recv_working_port()
    worker_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    worker_fd.setsockopt(socket.SOL_SOCKET, 
                        socket.SO_REUSEADDR, 1)
    worker_fd.connect((serv_info))
    hello = "connected to worker"
    try:
        bytes_sent = worker_fd.send(hello.encode('utf-8'))
        if bytes_sent == 0:
            print("no bytes sent to scheduler")
        else:
            print("hello sent")
    except IOError as send_err:
        print("send error", send_err)
        worker_fd.close()
        return