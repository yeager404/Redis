# Phase 1 - Manual Tests

## 1. Server Startup
- [ ] Server starts on port 6379.
- [ ] Invalid port is rejected.
- [ ] Starting on an occupied port reports an error.

## 2. TCP Connections
- [ ] One client can connect.
- [ ] Multiple clients can connect simultaneously.
- [ ] A disconnected client is removed.
- [ ] A new client can connect after a disconnect.

## 3. I/O behaviour
- [ ] Client data is echoed.
- [ ] Large message are echoed correctly.
- [ ] Multiple messages work on one connection.
- [ ] Partial writes preserve unsent output.

## 4. Resource handling
- [ ] Client disconnect does not terminate the server.
- [ ] No per-client threads are created.
- [ ] Closed connections release their file descriptors.
