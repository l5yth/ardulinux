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

/**
 * @file AsyncUDP.h
 *
 * Minimal AsyncUDP implementation for Linux, backed by libuv.
 *
 * Implements the bare minimum of the ESP-AsyncUDP API needed to run
 * meshcore's multicast UDP transport.  Only listenMulticast() and
 * writeTo() are provided; unicast listen, direct send, and the full
 * packet-source API are not implemented.
 *
 * The I/O loop runs on a dedicated background thread.  Packet delivery
 * callbacks are invoked from that thread, so handlers must be thread-safe
 * with respect to the Arduino main loop.
 *
 * Multicast loopback: IP_MULTICAST_LOOP is enabled so that multiple
 * processes on the same host can exchange packets over the same group.
 * To prevent a sender from receiving its own packet and re-processing it,
 * the implementation waits for the looped-back copy before sending the
 * next packet (see _waitingToBeLooped / _emptiedBuffer).
 */

#ifndef ESPASYNCUDP_H
#define ESPASYNCUDP_H

#include "IPAddress.h"
#include "Print.h"
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include <uv.h>

class AsyncUDP;

/**
 * A received UDP packet.
 *
 * Wraps a raw byte buffer and its length.  Instances are constructed
 * internally by AsyncUDP and passed by reference to the registered
 * packet handler — callers must not store a pointer to the packet
 * beyond the handler's return.
 */
class AsyncUDPPacket final
{
private:
    uint8_t *_data; ///< Pointer to received payload (owned by the receive callback)
    size_t _len;    ///< Length of the received payload in bytes

protected:
    /**
     * Construct a packet wrapping an existing buffer.
     *
     * @param byte Pointer to received data (not copied; caller retains ownership).
     * @param len  Length in bytes.
     */
    AsyncUDPPacket(uint8_t* byte, size_t len) {
        _data = byte;
        _len = len;
    };

public:
    /** Return a pointer to the raw packet payload. */
    uint8_t * data() {
        return _data;
    };

    /** Return the payload length in bytes. */
    size_t length() {
        return _len;
    };

    friend AsyncUDP;
};

/**
 * Internal send-queue entry.
 *
 * Holds a heap-allocated copy of the payload and addressing information
 * for one pending writeTo() call.  The copy is freed in the destructor.
 * Not part of the public API; used only by AsyncUDP internally.
 */
class asyncUDPSendTask final {
    protected:
        uint8_t *data; ///< Heap copy of the payload to send
        size_t len;    ///< Payload length in bytes
        IPAddress addr; ///< Destination IP address
        uint16_t port;  ///< Destination UDP port

    public:
        /**
         * Construct a send task, copying @p len bytes from @p data.
         *
         * @param data Source payload (copied into a heap buffer).
         * @param len  Number of bytes to copy.
         * @param addr Destination IP address.
         * @param port Destination UDP port.
         */
        asyncUDPSendTask(uint8_t *data, size_t len, IPAddress addr, uint16_t port);

        /** Free the heap-allocated payload copy. */
        ~asyncUDPSendTask() {
            free(data);
        };

    friend AsyncUDP;
};

/** Packet handler receiving only the packet (no user argument). */
typedef std::function<void(AsyncUDPPacket& packet)> AuPacketHandlerFunction;

/** Packet handler receiving a user-supplied void* argument and the packet. */
typedef std::function<void(void * arg, AsyncUDPPacket& packet)> AuPacketHandlerFunctionWithArg;

/**
 * Asynchronous UDP socket backed by a libuv event loop.
 *
 * Supports multicast listen/send on a single group address.  All I/O
 * runs on a private background thread; the public API (onPacket(),
 * writeTo()) is safe to call from any thread.
 *
 * Typical usage:
 * @code
 *   AsyncUDP udp;
 *   udp.onPacket([](AsyncUDPPacket& pkt) { ... });
 *   udp.listenMulticast(IPAddress(239,0,0,1), 4403);
 *   udp.writeTo(buf, len, IPAddress(239,0,0,1), 4403);
 * @endcode
 *
 * @note Only one multicast group per instance is supported.  Call
 *       listenMulticast() only once; subsequent calls return false.
 */
class AsyncUDP final
{
private:
    std::mutex _handlerMutex;           ///< Guards _handler for thread-safe registration
    AuPacketHandlerFunction _handler;   ///< Registered receive callback (may be null)

    std::mutex _sendQueueMutex;         ///< Guards _sendQueue
    /**
     * Pending send tasks.
     *
     * writeTo() enqueues tasks here; _attemptWrite() dequeues them one
     * at a time from the uv loop thread.  uv_udp_send is not thread-safe
     * so sends are serialised through this queue plus uv_async_send().
     */
    std::vector<std::unique_ptr<asyncUDPSendTask>> _sendQueue;

    /**
     * The task most recently sent, waiting for its multicast loopback copy.
     *
     * Because IP_MULTICAST_LOOP is enabled, every sent packet is also
     * received back on the same socket.  _waitingToBeLooped tracks which
     * task we are waiting to see looped back so we can suppress it from
     * the application-level handler.  Cleared once the looped copy is
     * matched in _DO_NOT_CALL_uv_on_read().
     */
    std::unique_ptr<asyncUDPSendTask> _waitingToBeLooped;

    /**
     * Set to true by _DO_NOT_CALL_uv_on_read() when the receive buffer
     * is exhausted while still waiting for a loopback.
     *
     * If the kernel dropped the looped packet (receive buffer full), the
     * timer callback uses this flag to declare the wait timed out and
     * unblock the send queue.
     */
    bool _emptiedBuffer = false;
    uv_timer_t _timer; ///< Watchdog timer for loopback detection timeout

    std::atomic<bool> _quit; ///< Set to true by the destructor to stop the uv loop
    std::thread _ioThread;   ///< Background thread running the libuv event loop

    bool _connected;         ///< True after listenMulticast() succeeds
    IPAddress _listenIP;     ///< The multicast group address we joined

    uv_loop_t _loop;   ///< libuv event loop (one per AsyncUDP instance)
    uv_udp_t _socket;  ///< libuv UDP handle wrapping _fd
    int _fd;           ///< Raw socket fd (managed by libuv after uv_udp_open)
    uv_async_t _async; ///< Inter-thread wakeup handle; signals the loop when the send queue changes

public:
    /** Initialise the libuv loop, async handle, and timer. */
    AsyncUDP();

    /**
     * Stop the I/O thread and close all libuv handles.
     *
     * Signals the loop thread via uv_async_send() then joins it.
     */
    ~AsyncUDP();

    /**
     * Register a packet-receive callback with an optional user argument.
     *
     * Thread-safe; may be called from any thread.  The previous callback
     * is replaced atomically.
     *
     * @param cb  Callback receiving a user argument and the packet.
     * @param arg Opaque pointer passed to @p cb on each received packet.
     */
    void onPacket(AuPacketHandlerFunctionWithArg cb, void * arg=NULL) {
        onPacket(std::bind(cb, arg, std::placeholders::_1));
    };

    /**
     * Register a packet-receive callback.
     *
     * Thread-safe; may be called from any thread.
     *
     * @param cb Callback invoked for each received packet that is not the
     *           looped-back copy of a locally sent packet.
     */
    void onPacket(AuPacketHandlerFunction cb) {
        _handlerMutex.lock();
        _handler = cb;
        _handlerMutex.unlock();
    };

    /**
     * Join a UDP multicast group and start receiving packets.
     *
     * Opens a raw SOCK_DGRAM socket, sets SO_REUSEADDR, SO_REUSEPORT,
     * SO_BROADCAST (for same-host multi-process delivery), IP_MULTICAST_TTL,
     * IP_MULTICAST_LOOP, then binds and joins the group.  Starts the uv
     * I/O loop on a background thread.
     *
     * @param addr Multicast group address to join.
     * @param port UDP port to bind.
     * @param ttl  IP TTL for outgoing multicast packets (default 1 = LAN only).
     * @return true on success, false if already connected or on setup failure.
     */
    bool listenMulticast(const IPAddress addr, uint16_t port, uint8_t ttl=1);

    /**
     * Enqueue a UDP datagram for asynchronous delivery.
     *
     * Thread-safe.  The payload is copied into a heap buffer owned by the
     * send task; the caller's buffer may be freed immediately after return.
     * Actual transmission happens on the I/O thread.
     *
     * @param data Pointer to payload bytes.
     * @param len  Payload length in bytes.
     * @param addr Destination IP address.
     * @param port Destination UDP port.
     * @return @p len (bytes enqueued; actual send may fail silently).
     */
    size_t writeTo(const uint8_t *data, size_t len, const IPAddress addr, uint16_t port);

    /** Return the multicast group address this socket is listening on. */
    IPAddress listenIP() {
        return _listenIP;
    };

    /** Return true if listenMulticast() has been called successfully. */
    operator bool() {
        return _connected;
    };

    /**
     * @name Internal libuv C-callback targets
     *
     * These methods are public only because libuv C callbacks need a plain
     * function pointer.  Do NOT call them directly.
     * @{
     */

    /**
     * Called by the libuv read callback for each received datagram.
     *
     * Matches received packets against the pending loopback task to suppress
     * self-sent packets, then forwards non-loopback packets to the registered
     * handler.
     */
    void _DO_NOT_CALL_uv_on_read(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);

    /** Called by the libuv async callback when the send queue may have new work. */
    void _DO_NOT_CALL_async_cb();

    /**
     * Called by the loopback watchdog timer.
     *
     * If the receive buffer was exhausted (_emptiedBuffer is set) without
     * matching the looped copy, declares the wait timed out and unblocks
     * the send queue.
     */
    void _DO_NOT_CALL_timer_cb();

    /** @} */

private:
    /**
     * Dequeue and send the next pending task, if any.
     *
     * Must be called from the uv loop thread.  Does nothing if a loopback
     * is still pending (_waitingToBeLooped is set).
     */
    void _attemptWrite();

    /**
     * Issue a uv_udp_send() for the given payload.
     *
     * Must be called from the uv loop thread.
     *
     * @param data Payload bytes (not copied; must remain valid until the
     *             send callback fires).
     * @param len  Payload length in bytes.
     * @param addr Destination IP address.
     * @param port Destination UDP port.
     */
    void _doWrite(const uint8_t *data, size_t len, const IPAddress addr, uint16_t port);
};

#endif
