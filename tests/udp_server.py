import socket
import datetime


# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind the socket to the port
server_address = ('172.28.43.216', 38500)
sock.bind(server_address)

while True:
    print('\nwaiting to receive message')
    data, address = sock.recvfrom(4096)
    now = datetime.datetime.now()
    print('%s: received %s bytes from %s' % (now.strftime("%Y-%m-%d %H:%M:%S"), len(data), address))
    print(data)
    if data:
        sent = sock.sendto(b'this is the host!', address)

# Don't forget to close the socket once done
sock.close()