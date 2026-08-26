from __future__ import annotations

import socket
import socketserver
import threading
import time

from app.config import SerialConfig, TcpConfig
from app.serial_client import SerialClient


class AckHandler(socketserver.StreamRequestHandler):
    def handle(self) -> None:
        line = self.rfile.readline().decode("ascii")
        request_id = line.split("ID=", 1)[1].split()[0]
        self.wfile.write(
            f"Ctrl:ACK ID={request_id} CMD=VOR.Status CODE=0 STATE=IDLE MSG=STATUS\r\n"
            .encode("ascii"))
        self.wfile.flush()


def test_tcp_transport_sends_and_matches_ack() -> None:
    server = socketserver.ThreadingTCPServer(("127.0.0.1", 0), AckHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    client = SerialClient(SerialConfig(port="", ack_timeout_seconds=1), 100,
                          TcpConfig("127.0.0.1", server.server_address[1], 1))
    try:
        client.connect()
        deadline = time.monotonic() + 1
        while not client.snapshot()["connected"] and time.monotonic() < deadline:
            time.sleep(0.01)
        assert client.snapshot()["transport"] == "tcp"
        assert client.command("Status").state == "IDLE"
    finally:
        client.close()
        server.shutdown()
        server.server_close()
