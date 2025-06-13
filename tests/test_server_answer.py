import socket
import threading
import argparse
import logging

OPCODE_REPLY = 2


def protocol_execution(sock):
    # 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
    # Get the length information (4 bytes)

    # Receive first 4 bytes which contain the length of Alice's name
    buf = sock.recv(4)
    # Convert received bytes to integer using big-endian format
    length = int.from_bytes(buf, "big")
    logging.info("[*] Length received: {}".format(length))

    # Get the name (Alice)
    # Receive the actual name using the length we just received
    buf = sock.recv(length)
    logging.info("[*] Name received: {}".format(buf.decode()))

    # 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
    # Send the length information (4 bytes)

    # Prepare Bob's name and its length
    name = "Bob"
    length = len(name)
    logging.info("[*] Length to be sent: {}".format(length))
    # Convert length to 4 bytes in big-endian format and send
    sock.send(int.to_bytes(length, 4, "big"))

    # Send the name (Bob)
    # Convert Bob's name to bytes and send
    logging.info("[*] Name to be sent: {}".format(name))
    sock.send(name.encode())

    # Implement following the instructions below
    # 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
    # The opcode should be 1

    # Receive 12 bytes containing opcode and two arguments
    buf = sock.recv(12)

    # The values are encoded in the big-endian style and should be translated into the little-endian style (because my machine follows the little-endian style)

    # Parse received data into opcode and two arguments
    # Each value is 4 bytes long
    opcode = int.from_bytes(buf[0:4], "little")
    arg1 = int.from_bytes(buf[4:8], "little")
    arg2 = int.from_bytes(buf[8:12], "little")

    logging.info("[*] Opcode: {}".format(opcode))
    logging.info("[*] Arg1: {}".format(arg1))
    logging.info("[*] Arg2: {}".format(arg2))

    # 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
    # The opcode should be 2

    # Calculate the sum of two arguments
    result = arg1 + arg2
    logging.info("[*] Result: {}".format(result))
    # Set opcode to 2 for reply
    opcode = 2
    # Send opcode and result in big-endian format
    sock.send(int.to_bytes(OPCODE_REPLY, 4, "big"))
    sock.send(int.to_bytes(result, 4, "big"))

    # Close the socket connection
    sock.close()


def run(addr, port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((addr, port))

    server.listen(2)
    logging.info("[*] Server is Listening on {}:{}".format(addr, port))

    while True:
        client, info = server.accept()

        logging.info(
            "[*] Server accept the connection from {}:{}".format(info[0], info[1])
        )

        client_handle = threading.Thread(target=protocol_execution, args=(client,))
        client_handle.start()


def command_line_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-a",
        "--addr",
        metavar="<server's IP address>",
        help="Server's IP address",
        type=str,
        default="0.0.0.0",
    )
    parser.add_argument(
        "-p",
        "--port",
        metavar="<server's open port>",
        help="Server's port",
        type=int,
        required=True,
    )
    parser.add_argument(
        "-l",
        "--log",
        metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>",
        help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)",
        type=str,
        default="INFO",
    )
    args = parser.parse_args()
    return args


def main():
    args = command_line_args()
    log_level = args.log
    logging.basicConfig(level=log_level)

    run(args.addr, args.port)


if __name__ == "__main__":
    main()
