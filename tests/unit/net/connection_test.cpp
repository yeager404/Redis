#include "redis/net/Connection.h"
#include "redis/net/ConnectionManager.h"

#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

using redis::net::Connection;
using redis::net::ConnectionManager;

int main() {
    // Test 1: Connection initialization with socketpair
    {
        int sv[2];
        int rc = ::socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        assert(rc == 0);

        {
            Connection conn(sv[0]);
            assert(conn.fd() == sv[0]);
            assert(!conn.peerClosed());

            // Check buffers
            assert(conn.inputBuffer().readableBytes() == 0);
            assert(conn.outputBuffer().readableBytes() == 0);

            conn.inputBuffer().append("inbound", 7);
            assert(conn.inputBuffer().readableBytes() == 7);

            conn.outputBuffer().append("outbound", 8);
            assert(conn.outputBuffer().readableBytes() == 8);

            conn.markPeerClosed();
            assert(conn.peerClosed());
        }
        // conn is destroyed here; sv[0] should be closed automatically by RAII
        assert(::fcntl(sv[0], F_GETFD) == -1 && errno == EBADF);
        ::close(sv[1]);
    }

    // Test 2: ConnectionManager lifecycle
    {
        int sv1[2];
        int sv2[2];
        assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv1) == 0);
        assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv2) == 0);

        ConnectionManager manager;

        // Verify non-existent descriptor
        assert(manager.get(sv1[0]) == nullptr);

        // Add sv1[0]
        Connection& conn1 = manager.add(sv1[0]);
        assert(conn1.fd() == sv1[0]);

        // Get sv1[0]
        Connection* retrieved1 = manager.get(sv1[0]);
        assert(retrieved1 != nullptr);
        assert(retrieved1->fd() == sv1[0]);

        // Add sv2[0]
        Connection& conn2 = manager.add(sv2[0]);
        assert(conn2.fd() == sv2[0]);
        assert(manager.get(sv2[0]) != nullptr);

        // Remove sv1[0]
        manager.remove(sv1[0]);
        assert(manager.get(sv1[0]) == nullptr);
        // Socket should have been closed when removed from manager
        assert(::fcntl(sv1[0], F_GETFD) == -1 && errno == EBADF);

        // sv2[0] should still be present
        assert(manager.get(sv2[0]) != nullptr);

        // Remove sv2[0]
        manager.remove(sv2[0]);
        assert(manager.get(sv2[0]) == nullptr);
        assert(::fcntl(sv2[0], F_GETFD) == -1 && errno == EBADF);

        ::close(sv1[1]);
        ::close(sv2[1]);
    }

    std::cout << "All Connection & ConnectionManager tests passed successfully!\n";
    return 0;
}
