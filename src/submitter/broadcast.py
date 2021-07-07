import socket
import struct
import traceback
from typing import Tuple

def broadcast_udp() -> int:
    '''
    This module handles the constant broadcast message from the client to the server
    The client sends a message to the scheduler to initiate conversation, giving the
    scheduler the submitter's IP address, and receives the scheduler's IP address and
    port in order to establish the connection for the work to be done

    this method does not take any arguements

    returns a tuple containing the scheduler's IP address and working port
    example: returns ('127.0.0.1', 8080)
    '''
    port = 31337
    broadcast = '127.255.255.255'

    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client.setsockopt(socket.SOL_SOCKET, 
                        socket.SO_REUSEADDR | socket.SO_BROADCAST, 1)

    while True:
        try:
            client.sendto(b'', (broadcast, port))
        except IOError as send_err:
            print("send error", send_err)
            traceback.print_exc()
            break

        try:
            data, temp = client.recvfrom(1024)
            if None is data:
                print("nothing received from scheduler")
            print("bytes received from scheduler")
            object = struct.unpack('!6s', data)
            port = object[0].decode('utf-8')
            return port
        except IOError as recv_err:
            print("recv error", recv_err)
            traceback.print_exc()

scheduler = broadcast_udp()
print("port ", scheduler)