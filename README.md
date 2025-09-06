# C HTTP Server

A minimal HTTP server written in C from scratch, showcasing low-level server concepts.

---

## Concepts covered

1. **Socket programming** — create/bind/listen/accept TCP connections.
2. **Multiple clients** — handle many connections at once.
3. **Basic HTTP request/response** — parse and reply to simple requests.
4. **Event-driven I/O** — use `epoll` with non-blocking sockets.
5. **Signal handling** — graceful shutdown with SIGINT.
6. **Threading** — worker threads to process requests concurrently.

---

## Quick start

```sh
make
./build/http-server -p 8080
```

Visit: [http://127.0.0.1:8080](http://127.0.0.1:8080)

---

## Testing

```sh
curl -v http://127.0.0.1:8080/

# load test
ab -n 1000 -c 50 http://127.0.0.1:8080/
```

---

## Code style & formatting

- Source code is automatically formatted using **GNU indent**.
- In VS Code, the **Run on Save** extension is used to apply formatting automatically.
- This ensures a consistent coding style across all files.

---

## Notes

- Default port: **8080** (override with `-p`).
- Use `Ctrl+C` to stop the server.
- Avoid running as root.

---
