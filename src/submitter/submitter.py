#!/usr/env/python3

import argparse
import re
from struct import pack

if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "submitter"

import modules.connect_to_scheduler as cts
import modules.help_msg as hm
import modules.job as job
import modules.pack_protocols as p_protocols

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



def handle_cmdline_arguments() -> list:
    """
    docstring goes here
    """
    cts.connect_to_scheduler()
    parser = argparse.ArgumentParser(description='enter operation chain to be processed.')
    parser.add_argument('-n', dest="options", nargs='+', help='comma delimited'\
                        'positive integers\n\tcan be singular or a valid range separated by'\
                        ' a hyphen')
    parser.add_argument('-s', dest="shutdown", type=None, nargs='?', help='meant to shutdown the '\
                        'scheduler\n\t does not take any arguments')
    args = parser.parse_args()
    return args

def handle_submitter() -> None:
    """
    docstring goes here
    """
    args = handle_cmdline_arguments()
    operation_list = args.options[0].split(' ')
    operation_chain = args.options[1].split(' ')
    if args.options == None or len(args.options) != 2:
        hm.usage_msg()
        exit()
    op_list = valid_operand_check(operation_list[0])
    print("operands:")
    print("\t", op_list)
    op_chain = valid_opchain_check(operation_chain[0])
    print("op_chain:")
    print("\t", op_chain)

handle_submitter()