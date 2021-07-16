#!/usr/env/python3
import struct
from typing import Tuple

class Packet_Protocol:
    """
    @brief - class to help the submitter and work applications appropriately
             create the communications packet to the scheduler
    """

    def __init__(self, version: int, operation: int):
        """
        @brief - initializes the packet's version and operation type
        @var 1 version - an integer (always resolving to 1) to represent
                         the packet communication's version
        @var 2 operation - an integer to represent the type of operation expected
                           to be performed by the scheduler. This also dictates the
                           packet's payload
        @return - N/A
        """

        self.version = int(version)
        self.operation = int(operation)

    def create_protocol_header(self, version: int, operation: int) -> Tuple:
        """
        @brief - creates the protocol header and calls the Byte's class
                 convert to bytes 
        @var 1 version - the protocol version (always 1)
        @var 2 operation - the operation to be performed (0 - Submit Job
                           3 - Query Work, 4 - Submit Work, 5 - Shutdown)
        @return - a tuple of bytes like objects containing the packet protol
                  version and operation
        """
        header: Tuple = ()

        job_packet = Bytes()

        header = job_packet.convert_to_bytes(version)
        header += job_packet.convert_to_bytes(operation)
        return header

class Bytes:
    """
    @brief a helper class to convert the header and payload into bytes
    """

    def __init__(self):
        """
        @brief - initializes the class
        """
        pass

    def convert_to_bytes(self, data_to_pack: int) -> bytes:
        """
        @brief - calls struct.pack to convert the integer value to a bytes
                 like object
        @var 1 data_to_pack - an integer value to convert to bytes
        @return - a bytes like object of the integer data to pass to the scheduler
        """
        packed_data = struct.pack('!I', data_to_pack)
        return packed_data

