#line 1 "D:\\Users\\person\\Documents\\vs\\wifiserialport\\WEB_SERIAL_CHANGES.md"
# Web serial bridge changes

- Initialize the web serial output queue and a thread-safe 1024-byte receive queue before starting the web server.
- Route WebSocket text input into the receive queue; `sertie` reads it and forwards it to `SERIAL2`.
- Report incoming bytes that cannot be queued instead of silently discarding them.
- Use the WebSocket server's managed client list and broadcast `SERIAL2` output to all connected clients.
- Batch queued serial output into WebSocket messages and service it from the main loop.
