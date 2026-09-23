#!/usr/bin/env python3
"""Small no-cache HTTP server for Vita update experiments."""

from __future__ import annotations

import argparse
import functools
import time
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class NoCacheHandler(SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    throttle_bytes_per_second = 0

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

    def log_message(self, format: str, *args: object) -> None:
        message = "%s - - %s\n" % (self.address_string(), format % args)
        print(message, end="", flush=True)
        with (Path(self.directory) / "access.log").open("a", encoding="utf-8") as log:
            log.write(message)

    def copyfile(self, source: object, outputfile: object) -> None:
        if (self.path.split("?", 1)[0] != "/payload.pkg" or
                self.throttle_bytes_per_second <= 0):
            super().copyfile(source, outputfile)
            return
        chunk_size = 32 * 1024
        while True:
            chunk = source.read(chunk_size)
            if not chunk:
                break
            outputfile.write(chunk)
            outputfile.flush()
            time.sleep(len(chunk) / self.throttle_bytes_per_second)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path("serve"))
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--throttle-kib", type=int, default=128,
                        help="limit payload.pkg transfer speed; zero disables throttling")
    args = parser.parse_args()
    root = args.root.resolve()
    if not root.is_dir():
        parser.error(f"server root does not exist: {root}")
    if args.throttle_kib < 0:
        parser.error("--throttle-kib cannot be negative")
    NoCacheHandler.throttle_bytes_per_second = args.throttle_kib * 1024
    handler = functools.partial(NoCacheHandler, directory=str(root))
    server = ThreadingHTTPServer((args.bind, args.port), handler)
    print(f"Serving {root} at http://{args.bind}:{args.port}/")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
