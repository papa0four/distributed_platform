import argparse
import socket

def usage_msg():
    print("usage: submitter.py -n [operand(s)] [op chain] [iterations (always 1)]")
    print("for single operand usage:")
    print("\tsubmitter.py -n 7 6-,|8,=>>4 1")
    print("for number ranges:")
    print("\texample: submitter.py -n 1-4 +3,-4,^6 1")
    print("for multi number requests:")
    print("\tsubmitter.py -n 1,4,7 +1,+2,-3,&37 1")

def handle_cmdline_arguments():
    parser = argparse.ArgumentParser(description='enter operation chain to be processed.')
    parser.add_argument('-n', dest="options", nargs='+', help='comma delimited'\
                        'positive integers\n\tcan be singular or a valid range separated by'\
                        ' a hyphen')
    parser.add_argument('-s', dest="shutdown", type=None, nargs='?', help='meant to shutdown the '\
                        'scheduler\n\t does not take any arguments')
    args = parser.parse_args()
    if len(args.options) != 3:
        usage_msg()
        exit()
    

handle_cmdline_arguments()