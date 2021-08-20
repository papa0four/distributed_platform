#!/usr/env/python3

import argparse
import re
from typing import Tuple
import sys

if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "submitter"

import modules.connect_to_scheduler as cts
import modules.help_msg as hm
import modules.pack_protocols as p_protocols

serv_info: Tuple = ()

def test_num_sz(num: int) -> bool:
    """
    @brief - a helper function to ensure any number passed is within the range of
             0 - 2^32
    @var 1 num - an integer value to check
    @return - true if valid, false if out of the above mentioned range
    """

    if abs(num) <= 0xffffffff:
        return True
    else:
        print(f" {num} is out of range 0 - 2^32, please enter a smaller number")
        exit()

def create_op_range(input_range: str) -> list:
    """
    @brief - a valid operand entry may consist of a abbreviated range, e.g.: 1-4
             this is a helper function to conver the range into a valid list of numbers
    @var 1 input_range - a str value consisting of the number range to convert
    @return - a list of numbers within the range specified
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
    @brief - a helper function to determine if the list of operands is valid
             e.g.: that they are appropriate delimitted and meet the requirement
             of 0 - 2^32
    @var 1 operands - a string containing all the operands passed by the command line
    @return - a list of each operand in their own list index
    """

    operand_list: list = []
    valid_input: str = "(-?[0-9]+(--?[0-9]+)?)"
    valid_input_range: str = "(-?[0-9]+--?[0-9]+)"
    if ',' not in operands:
        if re.fullmatch(valid_input, operands) is None:
            print(f"{operands} is not a valid value")
            return []
        elif '-' in operands:
            valid_range_check = '([0-9]+)(-?)([0-9]+)'
            if re.fullmatch(valid_input_range, operands) is not None:
                num_range = re.split(valid_range_check, operands)
                num_range = create_op_range(num_range)
                for num in num_range:
                    operand_list.append(num)
                return operand_list

        else:
            operand_list.append(operands)
            return operand_list

    for operand in operands.split(','):
        if re.fullmatch(valid_input, operand) is None:
            print(f"{operand} is not a valid value")
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
    @brief - a helper function to ensure the list of operations/operands to be
             performed on the operands list are valid. This includes a check on
             valid operations per the platform creation guidelines.
    @var 1 op_chain - a string of all operations to be performs on the operands list
                      given from the command line
    @return - a list of all operations groups to be performed
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

    if ',' not in op_chain:
        if re.fullmatch(valid_input, op_chain) is None:
            print(f"Invalid value single: {split_chain}\ttype: {type(split_chain)}")
            return []
    else:
        for split in split_chain.split(','):
            if re.fullmatch(valid_input, split) is None:
                print(f"Invalid value multiple: {split}\ttype: {type(split)}")
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

def create_payload(num_ops: int, op_chain: list, iter: int, 
                    num_items: int, items: list) -> bytes:
    """
    @brief - meant to create the submit job payload to be sent to the scheduler
             to parse and hand out the work to the workers
    @var 1 num_ops - an integer value of the number of operations in the op chain
    @var 2 op_chain - a list containing all of the operations to perform
    @var 3 iter - the number of iterations to perform (always 1)
    @var 4 num_items - the number of operands to perform the operations on
    @var 5 items - a list of all operands passed to the command line
    @return - a bytes like object containing all job data required to be sent to the
              scheduler
    """

    job_packet = p_protocols.Bytes()
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

def handle_submitter(conn_fd: int) -> None:
    """
    @brief - main submitter driver to receive the submitter application's 
             command line arguments, call all helper functions to appropriately
             generate the submit job packet, and send the submit job packet to 
             the scheduler for work
    @param(s) - N/A
    @return - N/A
    """
    protocol_version = 1
    submit_job = 0
    job_id_len = 4
    
    for i in range(1, len(sys.argv)):
        if sys.argv[i][0] == '-' and sys.argv[i][1].isdigit():
            sys.argv[i] = f" {sys.argv[i]}"
    parser = argparse.ArgumentParser(description='enter operation chain to be processed.')
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-n', dest="options", nargs='+', type=str)
    parser.add_argument('-s', dest="shutdown", action='store_true')
    parser.add_argument('-h', '--help', dest="help", action='store_true')
    args = parser.parse_args()
    
    if args.shutdown == True:
        shutdown = 5
        header = p_protocols.Packet_Protocol(protocol_version, shutdown)
        hdr = header.create_protocol_header(protocol_version, shutdown)
        try:
            bytes_sent = conn_fd.send(hdr)
            if (bytes_sent <= 0):
                print("could not send shutdown flag")
                exit()
        except IOError as senderr:
            print(f"could not establish send connection to scheduler: {senderr}")
            exit()
        print(f"sending shutdown flag...")
        exit()
    elif args.help == True:
        hm.usage_msg()
        exit()

    operation_list = args.options[0].split(' ')
    operation_chain = args.options[1].split(' ')
    iterations = args.options[2]
    if args.options == None or len(args.options) != 3:
        hm.usage_msg()
        exit()

    op_list = valid_operand_check(operation_list[0])
    if len(operation_chain) > 1:
        op_chain = valid_opchain_check(operation_chain[1])
    else:
        op_chain = valid_opchain_check(operation_chain[0])
    header = p_protocols.Packet_Protocol(protocol_version, submit_job)
    hdr = header.create_protocol_header(protocol_version, submit_job)
    payload = create_payload(len(op_chain), op_chain, iterations, 
                len(op_list), op_list)
    packet = hdr + payload
    try:
        bytes_sent = conn_fd.send(packet)
        if bytes_sent <= 0:
            print("no bytes sent to scheduler")
            exit()
        print(f"job submitted {len(packet)} bytes sent ...")
        bytes_recv = conn_fd.recv(job_id_len)
        job_id = int.from_bytes(bytes_recv, "big", signed=False)
        print(f"Job ID recv'd: {job_id}")
    except AttributeError:
        exit()

if __name__ == "__main__":
    conn_fd = cts.connect_to_scheduler()
    handle_submitter(conn_fd)
    conn_fd.close()