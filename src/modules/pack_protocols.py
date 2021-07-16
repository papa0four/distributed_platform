#!/usr/env/python3
import struct
from typing import Tuple

class Packet_Protocol:
    """
    docstring goes here
    """

    def __init__(self, version: int, operation: int):
        """
        docstring goes here
        """
        self.version = int(version)
        self.operation = int(operation)

    def create_protocol_header(self, version: int, operation: int) -> Tuple:
        """
        docstring goes here
        """
        header: Tuple = ()

        job_packet = Submit_Job()

        header = job_packet.convert_to_bytes(version)
        header += job_packet.convert_to_bytes(operation)
        return header

class Submit_Job:
    """
    docstring goes here
    """

    def __init__(self):
        """
        docstring goes here
        """
        pass

    @staticmethod
    def convert_to_bytes(data_to_pack: int) -> bytes:
        """
        docstring goes here
        """
        packed_data = struct.pack('!I', data_to_pack)
        return packed_data

class Query_Work:
    """
    docstring goes here
    """

    def __init__(self):
        """
        docstring goes here
        """
        pass


class Submit_Work:
    """
    docstring goes here
    """

    def __init__(self):
        """
        docstring goes here
        """
        pass

class Shutdown:
    """
    docstring goes here
    """
    def __init__(self):
        """
        docstring goes here
        """
        pass