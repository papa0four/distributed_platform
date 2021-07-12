#!/usr/env/python3
import struct

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
    def convert_to_bytes(data_to_pack: any) -> any:
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