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
    """
    brief - meant to receive the working socket established by the scheduler
            and return the connection for the submitter and worker to communicate
            with the scheduler via a TCP socket for all work
    param(s) - N/A
    return - returns the socket file descriptor for the connection to the
             scheduler
    """
    
    try:
        serv_info = bc.broadcast_and_recv_working_port()
        conn_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        conn_fd.setsockopt(socket.SOL_SOCKET, 
                            socket.SO_REUSEADDR, 1)
    except TypeError:
        exit()
    try:
        conn_fd.connect((serv_info))
        if conn_fd is None:
            print("connection unexpectedly closed...")
            exit()
        return conn_fd
    except TypeError:
        print(f"connection unexpectedly closed... ")
    
