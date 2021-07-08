import socket
import struct

def broadcast_and_recv_working_port() -> int:
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

    broadcast_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    broadcast_socket.setsockopt(socket.SOL_SOCKET, 
                        socket.SO_REUSEADDR | socket.SO_BROADCAST, 1)
    broadcast_socket.settimeout(5)

    while True:
        try:
            broadcast_socket.sendto(b'', (broadcast, port))
        except IOError as send_err:
            print("send error", send_err)
            return -1

        try:
            data, _ = broadcast_socket.recvfrom(1024)
            if None is data:
                print("nothing received from scheduler")
            print("bytes received from scheduler")
            object = struct.unpack('>H', data)
            port = object[0]
            return port
        except socket.error as error:
            print("broadcast connection timedout...", error)
            return -1

scheduler = broadcast_and_recv_working_port()
print(scheduler)