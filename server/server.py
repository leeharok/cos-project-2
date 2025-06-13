import socket
import requests
import threading
import argparse
import logging
import json
import sys
import struct

# Operation codes used for communication protocol
OPCODE_DATA = 1
OPCODE_WAIT = 2
OPCODE_DONE = 3
OPCODE_QUIT = 4

# Each vector ID maps to a model with a specific input dimension and index
VECTOR_INFO = {
    0: {"dim": 2, "index": 1},  # vec0: [discomfort_index, avg_power]
    1: {"dim": 3, "index": 2},  # vec1: [max_temp, avg_humid, avg_power]
    2: {"dim": 5, "index": 4},  # vec2: [max_humid, max_temp, month, year, avg_power]
}


class Server:
    def __init__(self, name, algorithm, port, caddr, cport, ntrain, ntest):
        self.name = name
        self.algorithm = algorithm
        self.port = port
        self.caddr = caddr  # AI module's IP address
        self.cport = cport  # AI module's port
        self.ntrain = ntrain
        self.ntest = ntest

        # Initialize AI models on the external AI module
        self._init_models()

        # Setup TCP socket server
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(("0.0.0.0", port))
        self.socket.listen(10)
        logging.info("[*] Server is listening on port {}".format(port))

        # Start listening for client connections in a separate thread
        threading.Thread(target=self.listener).start()

    def _init_models(self):
        # For each vector ID, request the AI module to create a model
        for vid, info in VECTOR_INFO.items():
            model_name = f"{self.name}_vec{vid}"
            url = f"http://{self.caddr}:{self.cport}/{model_name}"
            payload = {
                "algorithm": self.algorithm,
                "dimension": info["dim"],
                "index": info["index"],
            }
            try:
                res = requests.post(url, json=payload)
                if res.status_code == 200:
                    logging.info(f"[*] Created model: {model_name}")
                else:
                    logging.warning(
                        f"[!] Failed to create model {model_name} (status {res.status_code})"
                    )
            except Exception as e:
                logging.error(f"[!] Exception while creating model {model_name}: {e}")

    def listener(self):
        """
        Continuously listens for incoming client connections.
        For each connection, it spawns a new thread to handle the client.
        """
        while True:
            # Accept a new TCP connection
            client, addr = self.socket.accept()
            logging.info("[*] Connection from {}:{}".format(*addr))

            # Start a new thread to handle communication with this client
            threading.Thread(target=self.handler, args=(client,)).start()

    def parse_and_send(self, vector_id, buf, is_training):
        """
        Parse binary data buffer into float values and send it to the AI module
        via HTTP PUT request. Used for both training and testing.
        """
        # Validate the vector ID
        if vector_id not in VECTOR_INFO:
            logging.error("Invalid vector ID: {}".format(vector_id))
            return

        dim = VECTOR_INFO[vector_id]["dim"]

        # Validate payload size (must be dim * 4 bytes since each float is 4 bytes)
        if len(buf) != dim * 4:
            logging.error(
                f"Payload length mismatch for vec{vector_id}: expected {dim*4}, got {len(buf)}"
            )
            return

        # Unpack the binary buffer into float values (big-endian format)
        fmt = f">{dim}f"
        values = list(struct.unpack(fmt, buf))
        logging.info(f"[vec{vector_id}] Received values: {values}")

        # Build the model name and target endpoint
        model_name = f"{self.name}_vec{vector_id}"
        endpoint = "training" if is_training else "testing"
        url = f"http://{self.caddr}:{self.cport}/{model_name}/{endpoint}"

        # Send the data to the AI module for training or testing
        response = requests.put(url, json=json.dumps({"value": values}))
        result = response.json()

        # Log error if AI module reports failure
        if result.get("opcode") == "failure":
            logging.error(
                f"Failed to send data to AI module: {result.get('reason', 'unknown')}"
            )

    def handler(self, client):
        """
        Handles communication with a single client.
        It processes both training and testing data, sends them to the AI module,
        and retrieves the final results.
        """
        for is_training, count in [(True, self.ntrain), (False, self.ntest)]:
            for _ in range(count):
                # Read 2-byte header: [opcode (1 byte), vector_id (1 byte)]
                header = client.recv(2)
                if len(header) < 2:
                    logging.error("Incomplete header")
                    return
                opcode, vector_id = header[0], header[1]

                # Only OPCODE_DATA is accepted at this point
                if opcode != OPCODE_DATA:
                    logging.error("Invalid opcode: {}".format(opcode))
                    return

                # Get the expected payload dimension for this vector ID
                dim = VECTOR_INFO.get(vector_id, {}).get("dim", 0)
                if dim == 0:
                    logging.error(f"Unknown vector ID: {vector_id}")
                    return

                # Read the payload containing float values (dim * 4 bytes)
                payload = client.recv(dim * 4)
                if len(payload) != dim * 4:
                    logging.error("Incomplete payload")
                    return

                # Parse and send the payload to the AI module
                self.parse_and_send(vector_id, payload, is_training)

                # Notify the client that this data point is processed
                client.send(OPCODE_DONE.to_bytes(1, "big"))

            if is_training:
                # After training data is sent, notify the AI module to train the model
                for vid in VECTOR_INFO:
                    url = f"http://{self.caddr}:{self.cport}/{self.name}_vec{vid}/training"
                    try:
                        requests.post(url)
                    except:
                        logging.warning(f"Training failed for vec{vid}")

        # Notify the client that all data is processed
        client.send(OPCODE_QUIT.to_bytes(1, "big"))

        # Retrieve final testing results from AI module for all vector types
        for vid in VECTOR_INFO:
            model_name = f"{self.name}_vec{vid}"
            url = f"http://{self.caddr}:{self.cport}/{model_name}/result"
            try:
                response = requests.get(url).json()
                if response.get("opcode") == "success":
                    logging.info(
                        f"[Result - vec{vid}] Accuracy: {response['accuracy']}%, Correct: {response['correct']}, Incorrect: {response['incorrect']}"
                    )
                else:
                    logging.warning(
                        f"[vec{vid}] Failed to get result: {response.get('reason', 'unknown')}"
                    )
            except Exception as e:
                logging.warning(f"[vec{vid}] Error retrieving result: {e}")


def parse_args():
    # Parse command-line arguments for server configuration
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True)
    parser.add_argument("--algorithm", required=True)
    parser.add_argument("--lport", type=int, required=True)
    parser.add_argument("--caddr", required=True)
    parser.add_argument("--cport", type=int, required=True)
    parser.add_argument("--ntrain", type=int, default=10)
    parser.add_argument("--ntest", type=int, default=10)
    return parser.parse_args()


def main():
    args = parse_args()
    logging.basicConfig(level=logging.INFO)

    # Create the server with the provided settings
    Server(
        args.name,
        args.algorithm,
        args.lport,
        args.caddr,
        args.cport,
        args.ntrain,
        args.ntest,
    )


if __name__ == "__main__":
    main()
