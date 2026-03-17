// ArduLinux - Arduino API for Linux
// Copyright (c) 2011-19 Arduino LLC.
// Copyright (c) 2020-23 Geeksville Industries, LLC
// Copyright (c) 2024-26 jp-bennett
// Copyright (c) 2026-27 l5yth
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include "AsyncUDP.h"
#include <unistd.h>
#include <sys/socket.h>
#include "Utility.h"

/** How long the loopback watchdog waits before giving up (milliseconds). */
#define LOOP_TIMER_CHECK_TIMEOUT_MS 100

// libuv C-callback adapters: libuv requires plain function pointers, so these
// free functions extract the AsyncUDP* stored in handle->data and forward to
// the corresponding member function.

void _asyncudp_async_cb(uv_async_t *handle) {
    AsyncUDP *udp = (AsyncUDP *)handle->data;
    udp->_DO_NOT_CALL_async_cb();
}

void _asyncudp_timer_cb(uv_timer_t *handle) {
    AsyncUDP *udp = (AsyncUDP *)handle->data;
    udp->_DO_NOT_CALL_timer_cb();
}

AsyncUDP::AsyncUDP() {
    _handler = NULL;
    _connected = false;
    _fd = 0;
    _quit.store(false);

    // Initialise the event loop and register the cross-thread wakeup handle.
    // The async handle allows writeTo() (called from the Arduino thread) to
    // wake the I/O thread when a new send task is enqueued.
    uv_loop_init(&_loop);
    _async.data = this;
    uv_async_init(&_loop, &_async, _asyncudp_async_cb);
    _timer.data = this;
    uv_timer_init(&_loop, &_timer);
}

AsyncUDP::~AsyncUDP() {
    // Signal the loop thread to stop, then wait for it to finish.
    _quit.store(true);
    uv_async_send(&_async);  // Wake the loop so it sees _quit == true
    _ioThread.join();
    uv_loop_close(&_loop);
    if (_fd > 0) {
        close(_fd);
        _fd = 0;
    }
}

asyncUDPSendTask::asyncUDPSendTask(uint8_t *data, size_t len, IPAddress addr, uint16_t port) {
    // Copy the payload so the caller can free their buffer immediately.
    this->data = (uint8_t*)malloc(len);
    memcpy(this->data, data, len);
    this->len = len;
    this->addr = addr;
    this->port = port;
}

/** libuv buffer allocation callback: provide a fresh heap buffer for each received datagram. */
void _asyncudp_alloc_buffer_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char *)malloc(suggested_size);
    buf->len = suggested_size;
}

/** libuv UDP read callback adapter: forwards to the AsyncUDP member function. */
void _asyncudp_on_read_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    AsyncUDP *udp = (AsyncUDP *)handle->data;
    udp->_DO_NOT_CALL_uv_on_read(handle, nread, buf, addr, flags);
}

void AsyncUDP::_DO_NOT_CALL_uv_on_read(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    if (nread <= 0) {
        if (_waitingToBeLooped) {
            // We are waiting to receive a packet yet we just exhausted the receive buffer.
            // This can happen if we are unlucky and this happens to run while we were sending a packet.
            // Or this happens because the receive buffer was full and the packet we are waiting for was dropped.
            _emptiedBuffer = true;
        }
        return;
    }
    _handlerMutex.lock();
    auto h = _handler;
    _handlerMutex.unlock();
    if (_waitingToBeLooped && nread == _waitingToBeLooped->len && memcmp(buf->base, _waitingToBeLooped->data, nread) == 0) {
        // This packet matches what we sent — it is our own loopback copy.
        // Suppress it from the handler and unblock the send queue.
        _waitingToBeLooped = std::unique_ptr<asyncUDPSendTask>();
        uv_timer_stop(&_timer);
        _attemptWrite();
    } else {
        // Not a loopback copy; deliver to the application handler.
        if (h) {
            AsyncUDPPacket packet((uint8_t*)buf->base, nread);
            h(packet);
        }
    }
    free(buf->base);
}

bool AsyncUDP::listenMulticast(const IPAddress addr, uint16_t port, uint8_t ttl) {
    if (_connected) {
        return false;
    }
    if (uv_udp_init(&_loop, &_socket) < 0) {
        return false;
    }
    _socket.data = this;

    // Create a raw non-blocking socket outside libuv so we can set socket
    // options before handing it to the uv_udp handle via uv_udp_open().
    _fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
    if (_fd < 0) {
        return false;
    }

    // SO_REUSEADDR and SO_REUSEPORT allow multiple processes on the same host
    // to bind to the same multicast port and each receive all packets.
    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
        // FIXME: show a message, this is not a killer but it will prevent multiple instances to share the same port
    }
    opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(int)) < 0) {
        // FIXME: show a message, this is not a killer but it will prevent multiple instances to share the same port
    }
    // On Linux, setting SO_BROADCAST has the undocumented side effect to change the loadbalance
    // behavior into a broadcast where all sockets receive all packets, which we want.
    // We never want to send packets to the broadcast address, which is the documented behavior.
    opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(int)) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }

    // Limit how far multicast packets travel; TTL=1 means LAN only.
    opt = ttl;
    if (setsockopt(_fd, IPPROTO_IP, IP_MULTICAST_TTL, &opt, sizeof(opt)) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }
    // Enable multicast loopback so that multiple processes on the same host
    // all receive packets sent to the group (including our own sends).
    // The loopback suppression logic in _DO_NOT_CALL_uv_on_read() prevents
    // the sender from re-processing its own packets.
    opt = 1;
    if (setsockopt(_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt)) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }

    // Hand the raw socket to libuv for async I/O management.
    if (uv_udp_open(&_socket, _fd) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }

    // FIXME: don't do bytes → string → bytes IP conversion
    int maxIpLength = 3*4+3; // 3 digits per octet, 4 octets, 3 dots
    char addr_str[maxIpLength+1]; // +1 for null terminator
    snprintf(addr_str, maxIpLength, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    addr_str[maxIpLength] = '\0';
    struct sockaddr uvAddr;
    uv_ip4_addr(addr_str, port, (struct sockaddr_in *)&uvAddr);

    if (uv_udp_set_membership(&_socket, addr_str, NULL, UV_JOIN_GROUP) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }
    if (uv_udp_bind(&_socket, (const struct sockaddr *)&uvAddr, 0) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }
    if (uv_udp_recv_start(&_socket, _asyncudp_alloc_buffer_cb, _asyncudp_on_read_cb) < 0) {
        close(_fd);
        _fd = 0;
        return true;
    }

    // Start the libuv event loop on a dedicated background thread.
    _ioThread = std::thread([this](){
        uv_run(&_loop, UV_RUN_DEFAULT);
    });

    _listenIP = addr;
    _connected = true;
    return true;
}

size_t AsyncUDP::writeTo(const uint8_t *data, size_t len, const IPAddress addr, uint16_t port) {
    // Enqueue a heap copy of the payload and wake the I/O thread.
    // uv_udp_send is not thread-safe, so the actual send happens in _attemptWrite()
    // which runs on the uv loop thread.
    auto task = std::make_unique<asyncUDPSendTask>((uint8_t*)data, len, addr, port);
    _sendQueueMutex.lock();
    _sendQueue.push_back(std::move(task));
    _sendQueueMutex.unlock();
    uv_async_send(&_async);  // Wake the I/O thread
    return len;
}

void AsyncUDP::_attemptWrite() {
    if (_waitingToBeLooped) {
        // Still waiting for the loopback copy of the previous send; don't
        // send the next packet yet to avoid interleaving loopback detection.
        return;
    }
    _sendQueueMutex.lock();
    if (!_sendQueue.empty()) {
        auto task = std::move(_sendQueue.back());
        _sendQueue.pop_back();
        _sendQueueMutex.unlock();
        _doWrite(task->data, task->len, task->addr, task->port);
        // Park the task so the read callback can match its loopback copy.
        _waitingToBeLooped = std::move(task);
        _emptiedBuffer = false;
        // Start a watchdog timer in case the loopback copy is dropped.
        uv_timer_start(&_timer, _asyncudp_timer_cb, LOOP_TIMER_CHECK_TIMEOUT_MS, LOOP_TIMER_CHECK_TIMEOUT_MS);
    } else {
        _sendQueueMutex.unlock();
    }
}

void AsyncUDP::_DO_NOT_CALL_async_cb() {
    _attemptWrite();
    if (_quit.load()) {
        // Destructor was called; leave the multicast group and stop the loop.
        uv_udp_recv_stop(&_socket);
        // FIXME: don't do bytes → string → bytes IP conversion
        int maxIpLength = 3*4+3; // 3 digits per octet, 4 octets, 3 dots
        char addr_str[maxIpLength+1]; // +1 for null terminator
        snprintf(addr_str, maxIpLength, "%d.%d.%d.%d", _listenIP[0], _listenIP[1], _listenIP[2], _listenIP[3]);
        addr_str[maxIpLength] = '\0';
        uv_udp_set_membership(&_socket, addr_str, NULL, UV_LEAVE_GROUP);
        uv_stop(&_loop);
    }
}

void AsyncUDP::_DO_NOT_CALL_timer_cb() {
    if (!_waitingToBeLooped) {
        uv_timer_stop(&_timer);
        return;
    }
    if (_emptiedBuffer) {
        // We waited LOOP_TIMER_CHECK_TIMEOUT_MS ms and exhausted the receive
        // buffer without seeing the looped copy — it was likely dropped by the
        // kernel (receive buffer full).  Give up waiting and unblock the queue.
        _waitingToBeLooped = std::unique_ptr<asyncUDPSendTask>();
        uv_timer_stop(&_timer);
    } else {
        // The receive buffer is not yet exhausted; keep waiting.
    }
}

/** libuv send completion callback: frees the heap-allocated request struct. */
void _asyncudp_send_cb(uv_udp_send_t *req, int status) {
    free(req);
}

void AsyncUDP::_doWrite(const uint8_t *data, size_t len, const IPAddress addr, uint16_t port) {
    // FIXME: don't do bytes → string → bytes IP conversion
    int maxIpLength = 3*4+3; // 3 digits per octet, 4 octets, 3 dots
    char addr_str[maxIpLength+1]; // +1 for null terminator
    snprintf(addr_str, maxIpLength, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    addr_str[maxIpLength] = '\0';

    // FIXME: implement error handling rather than raising SIGSEGV
    struct sockaddr uvAddr;
    uv_ip4_addr(addr_str, port, (struct sockaddr_in *)&uvAddr);

    // The uv_udp_send_t request struct must outlive the call; free it in the callback.
    uv_udp_send_t *req = (uv_udp_send_t *)malloc(sizeof(uv_udp_send_t));
    uv_buf_t msg;
    msg.base = (char *)data;
    msg.len = len;
    if (uv_udp_send(req, &_socket, &msg, 1, (const struct sockaddr *)&uvAddr, _asyncudp_send_cb) < 0) {
        // FIXME: I don't know how to handle this error, better to just drop the packet.
    }
}
