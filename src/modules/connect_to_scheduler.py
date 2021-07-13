#!/usr/bin/env python3

import socket
from typing import Tuple


if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "connect_to_scheduler"

import modules.broadcast as bc

def connect_to_scheduler() -> int:
    serv_info = bc.broadcast_and_recv_working_port()
    conn_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn_fd.setsockopt(socket.SOL_SOCKET, 
                        socket.SO_REUSEADDR, 1)
    conn_fd.connect((serv_info))
    print(f"connection made to scheduler via {serv_info[0]}:{serv_info[1]}")
    return conn_fd
