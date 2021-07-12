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
    conn_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn_fd.setsockopt(socket.SOL_SOCKET, 
                        socket.SO_REUSEADDR, 1)
    conn_fd.connect((serv_info))
    hello = "client connected ..."
    try:
        bytes_sent = conn_fd.send(hello.encode('utf-8'))
        if bytes_sent == 0:
            print("no bytes sent to scheduler")
        else:
            print(f"connection made to scheduler via {serv_info[0]}:{serv_info[1]}")
    except IOError as send_err:
        print("send error", send_err)
        conn_fd.close()
        return