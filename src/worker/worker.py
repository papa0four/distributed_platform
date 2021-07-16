#!/usr/bin/env python3

import struct
from typing import Dict, Tuple
import socket
import time

if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "worker"

import modules.connect_to_scheduler as cts
import modules.pack_protocols as p_protocols

def unpack_work(work: bytes) -> Dict:
    task = struct.unpack('<' + ('I' * (len(work)//4)), work)
    item = task[0]
    num_ops = task[1]
    op_chain = task[2:-1]
    iterations = task[-1]
    task_list = {
        "Item": item,
        "Num Ops": num_ops,
        "Op Chain": op_chain,
        "Iters": iterations
    }
    return task_list

def computation(item: int, num_ops: int, op_chain: Tuple, iterations: int) -> int:
    #iterations
    for n in range(iterations):
        #iterate over every operation in op_chain
        for i in range(0, num_ops * 2, 2):
            # ADD
            if op_chain[i] == 0:
                item = item + int(op_chain[i + 1])
            elif op_chain[i] == 1:
                item = item - int(op_chain[i + 1])
            elif op_chain[i] == 2:
                item = int(op_chain[i + 1]) - item
            elif op_chain[i] == 3:
                item = item & int(op_chain[i + 1])
            elif op_chain[i] == 4:
                item = item | int(op_chain[i + 1])
            elif op_chain[i] == 5:
                item = item ^ int(op_chain[i + 1])
            elif op_chain[i] == 6:
                item = ~item
            elif op_chain[i] == 7:
                item = item >> int(op_chain[i + 1])
            elif op_chain[i] == 8:
                item = item << int(op_chain[i + 1])
    print("computation complete...")
    return item

def handle_work() -> Tuple:
    VERSION = 1
    QUERY_WORK = 3
    item = 0
    num_ops = 0
    op_chain: Tuple = ()
    iterations = 0
    try:
        conn_fd = cts.connect_to_scheduler()
        if conn_fd == None:
            exit()
    except IOError:
        exit()
    header = p_protocols.Packet_Protocol(VERSION, QUERY_WORK)
    hdr = header.create_protocol_header(VERSION, QUERY_WORK)
    try:
        bytes_sent = conn_fd.send(hdr)
        if bytes_sent <= 0:
            print("could not query for work")
    except IOError as send_err:
        print(f"an IO Error occurred: {send_err}")

    try:
        work = conn_fd.recv(1024)
        if work == b'':
            print("scheduler shut down, termination flag received")
            exit()
        task_list = unpack_work(work)
        for key, val in task_list.items():
            if key == "Op Chain":
                op_chain = val
            elif key == "Item":
                item = val
            elif key == "Num Ops":
                num_ops = val
            elif key == "Iters":
                iterations = val
        
        answer = computation(item, num_ops, op_chain, iterations)
        return (conn_fd, answer)
    except IOError as recv_err:
        print(f"could not receive work: {recv_err}")
        exit()

def submit_work_answer() -> int:
    conn_fd, answer = handle_work()
    VERSION = 1
    SUBMIT_WORK = 4

    header = p_protocols.Packet_Protocol(VERSION, SUBMIT_WORK)
    hdr = header.create_protocol_header(VERSION, SUBMIT_WORK)
    answer = answer.to_bytes(4, 'little')
    packet = hdr + answer
    try:
        bytes_sent = conn_fd.send(packet)
        if bytes_sent <= 0:
            print("did not send answer to scheduler")
        return conn_fd
    except (IOError, socket.error):
        print(f"an error occurred sending answer")
        return -1

def main():
    running = True
    sleep = 1
    while(running):
        try:
            conn_fd = submit_work_answer()
            if -1 == conn_fd:
                running = False
            time.sleep(sleep)
        except ConnectionError as conn_err:
            print(f"connection error occurred: {conn_err}")
            running = False
    
    conn_fd.close()

if __name__ == "__main__":
    main()
        