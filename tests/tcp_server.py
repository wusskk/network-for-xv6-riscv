import socket
import time
import datetime

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('172.28.43.216', 38600)
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print('waiting for a connection')
    connection, client_address = sock.accept()

    # try:
    now = datetime.datetime.now()
    print(now.strftime("%Y-%m-%d %H:%M:%S"), ': connection from', client_address)

    # Receive data
    data = connection.recv(4096)
    print('received "%s"' % data.decode('utf-8'))

    # Send data
    message = 'this is the host!'
    connection.sendall(message.encode('utf-8'))

    #     # Wait for 10 seconds
    #     time.sleep(10)

    # finally:
    #     # Clean up the connection
    #     connection.close()

# Don't forget to close the socket once done
sock.close()