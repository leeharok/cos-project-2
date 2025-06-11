import socket
import requests
import threading
import argparse
import logging
import json
import sys
import struct

OPCODE_DATA = 1
OPCODE_WAIT = 2
OPCODE_DONE = 3
OPCODE_QUIT = 4

VECTOR_INFO = {
    0: {"dim": 2, "index": 1},  # discomfort_index, avg_power
    1: {"dim": 3, "index": 2},  # max_temp, avg_humid, avg_power
    2: {"dim": 5, "index": 4},  # max_humid, max_temp, month, year, avg_power
}


class Server:
    def __init__(self, name, algorithm, port, caddr, cport, ntrain, ntest):
        self.name = name
        self.algorithm = algorithm
        self.port = port
        self.caddr = caddr
        self.cport = cport
        self.ntrain = ntrain
        self.ntest = ntest

        self._init_models()

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(("0.0.0.0", port))
        self.socket.listen(10)
        logging.info("[*] Server is listening on port {}".format(port))
        threading.Thread(target=self.listener).start()

    def _init_models(self):
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
        while True:
            client, addr = self.socket.accept()
            logging.info("[*] Connection from {}:{}".format(*addr))
            threading.Thread(target=self.handler, args=(client,)).start()

    def parse_and_send(self, vector_id, buf, is_training):
        if vector_id not in VECTOR_INFO:
            logging.error("Invalid vector ID: {}".format(vector_id))
            return

        dim = VECTOR_INFO[vector_id]["dim"]
        if len(buf) != dim * 4:
            logging.error(
                f"Payload length mismatch for vec{vector_id}: expected {dim*4}, got {len(buf)}"
            )
            return

        fmt = f">{dim}f"
        values = list(struct.unpack(fmt, buf))
        logging.info(f"[vec{vector_id}] Received values: {values}")
        # a = int.from_bytes(buf[0:4], byteorder="big")
        # b = int.from_bytes(buf[4:8], byteorder="big")
        # c = int.from_bytes(buf[8:12], byteorder="big")
        # d = int.from_bytes(buf[12:16], byteorder="big")
        # e = int.from_bytes(buf[16:20], byteorder="big")
        # values = [a, b, c, d, e]

        model_name = f"{self.name}_vec{vector_id}"
        endpoint = "training" if is_training else "testing"
        url = f"http://{self.caddr}:{self.cport}/{model_name}/{endpoint}"
        response = requests.put(url, json=json.dumps({"value": values}))
        result = response.json()

        if result.get("opcode") == "failure":
            logging.error(
                f"Failed to send data to AI module: {result.get('reason', 'unknown')}"
            )

    def handler(self, client):
        for is_training, count in [(True, self.ntrain), (False, self.ntest)]:
            for _ in range(count):
                header = client.recv(2)
                if len(header) < 2:
                    logging.error("Incomplete header")
                    return
                opcode, vector_id = header[0], header[1]

                if opcode != OPCODE_DATA:
                    logging.error("Invalid opcode: {}".format(opcode))
                    return

                dim = VECTOR_INFO.get(vector_id, {}).get("dim", 0)
                if dim == 0:
                    logging.error(f"Unknown vector ID: {vector_id}")
                    return

                payload = client.recv(dim * 4)
                if len(payload) != dim * 4:
                    logging.error("Incomplete payload")
                    return

                self.parse_and_send(vector_id, payload, is_training)
                client.send(OPCODE_DONE.to_bytes(1, "big"))

            if is_training:
                for vid in VECTOR_INFO:
                    url = f"http://{self.caddr}:{self.cport}/{self.name}_vec{vid}/training"
                    try:
                        requests.post(url)
                    except:
                        logging.warning(f"Training failed for vec{vid}")

        client.send(OPCODE_QUIT.to_bytes(1, "big"))

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
