import socket
conn = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
conn.connect(('localhost',8402))
conn.send("get a\r")
conn.send("\nget bb\r\n")
print conn.recv(3)

