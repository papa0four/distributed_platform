#!/usr/env/python3

import argparse
import re
from typing import Tuple

if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "submitter"

import modules.connect_to_scheduler as cts
import modules.help_msg as hm
import modules.job as job
import modules.pack_protocols as p_protocols

serv_info: Tuple = ()

def test_num_sz(num: int) -> bool:
    """
    docstring goes here
    """
    if abs(num) <= 0xffffffff:
        return True
    else:
        print(f" {num} is out of range 0 - 2^32, please enter a smaller number")
        return False

def create_op_range(input_range: str) -> list:
    """
    docstring goes here
    """
    range_list: list = []
    min = input_range[1]
    max = input_range[3]
    for num in range(int(min), int(max) + 1):
        if test_num_sz(int(num)) == True:
            range_list.append(num)
        else:
            print(num, " is out of range 0 - 2^32, please enter a smaller number")
            return []
    return range_list


def valid_operand_check(operands: str) -> list:
    """
    docstring goes here
    """
    operand_list: list = []
    valid_input: str = "(-?[0-9]+(--?[0-9]+)?)"
    valid_input_range: str = "(-?[0-9]+--?[0-9]+)"
    for operand in operands.split(','):
        if re.fullmatch(valid_input, operand) is None:
            print(operand, " is not a valid value")
            return []
        else:
            valid_range_check = '([0-9]+)(-?)([0-9]+)'
            if re.fullmatch(valid_input_range, operand) is not None:
                num_range = re.split(valid_range_check, operand)
                num_range = create_op_range(num_range)
                for num in num_range:
                    operand_list.append(num)
                continue
            elif test_num_sz(int(operand)) == True:
                operand_list.append(int(operand))
    return operand_list


def valid_opchain_check(op_chain: str) -> list:
    """
    docstring goes here
    """
    valid_operations = {
        "+": 0, "-": 1, "~": 6, "=<<": 8, "=>>": 7,
        "&": 3, "^": 5, "|": 4
    }
    valid_input = re.compile(r"(([0-9]+|\+|-|=>>|=<<|&|\^|\||){1}([0-9]+|-{1}|~))")
    operation: str
    operand: str
    op_group: tuple
    chain_of_ops: list = []
    split_chain = op_chain

    for split in split_chain.split(','):
        if re.fullmatch(valid_input, split) is None:
            print(f"Invalid value: {split}")
            return []

    for item in valid_input.finditer(op_chain):
        if item.group(3) == '-':
            operation = 2
            operand = item.group(2)
            op_group = (operation, int(operand))
        elif item.group(3) == '~':
            operand = 0
            operation = 6
            op_group = (operation, int(operand))
        else:
            operation = valid_operations.get(item.group(2))
            operand = item.group(3)
            op_group = (operation, int(operand))
        chain_of_ops.append(op_group)
    return chain_of_ops

def create_protocol_header(version: int, operation: int) -> Tuple:
    """
    docstring goes here
    """
    header: Tuple = ()

    protocol_header = p_protocols.Packet_Protocol(version, operation)
    job_packet = p_protocols.Submit_Job()

    header = job_packet.convert_to_bytes(protocol_header.version)
    header += job_packet.convert_to_bytes(protocol_header.operation)
    return header

def create_payload(num_ops: int, op_chain: list, iter: int, 
                    num_items: int, items: list) -> bytes:
    """
    docstring goes here
    """
    job_packet = p_protocols.Submit_Job()
    payload = job_packet.convert_to_bytes(int(num_ops))

    for _, group in enumerate(op_chain):
        operation = group[0]
        operand = group[1]
        payload += job_packet.convert_to_bytes(operation)
        payload += job_packet.convert_to_bytes(operand)

    payload += job_packet.convert_to_bytes(int(iter))
    payload += job_packet.convert_to_bytes(int(num_items))

    for operand in items:
        payload += job_packet.convert_to_bytes(int(operand))

    return payload

def handle_submitter() -> None:
    """
    docstring goes here
    """
    protocol_version = 1
    submit_job = 0
    iterations = 1
    conn_fd = cts.connect_to_scheduler()
    parser = argparse.ArgumentParser(description='enter operation chain to be processed.')
    parser.add_argument('-n', dest="options", nargs='+', help='comma delimited'\
                        'positive integers\n\tcan be singular or a valid range separated by'\
                        ' a hyphen')
    parser.add_argument('-s', dest="shutdown", action='store_true', help='meant to shutdown the '\
                        'scheduler\n\t does not take any arguments')
    args = parser.parse_args()
    
    if args.shutdown == True:
        shutdown = 5
        header = create_protocol_header(protocol_version, shutdown)
        print(f"sending shutdown flag...")
        exit()

    operation_list = args.options[0].split(' ')
    operation_chain = args.options[1].split(' ')
    if args.options == None or len(args.options) != 2:
        hm.usage_msg()
        exit()

    op_list = valid_operand_check(operation_list[0])
    op_chain = valid_opchain_check(operation_chain[0])
    header = create_protocol_header(protocol_version, submit_job)
    payload = create_payload(len(op_chain), op_chain, iterations, 
                len(op_list), op_list)
    packet = header + payload
    try:
        bytes_sent = conn_fd.send(packet)
        if bytes_sent <= 0:
            print("no bytes sent to scheduler")
        print(f"job submitted {len(packet)} bytes sent ...")
    except IOError as send_err:
        print("send error", send_err)
        conn_fd.close()
        exit()

handle_submitter()