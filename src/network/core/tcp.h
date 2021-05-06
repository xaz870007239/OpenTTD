/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file tcp.h Basic functions to receive and send TCP packets.
 */

#ifndef NETWORK_CORE_TCP_H
#define NETWORK_CORE_TCP_H

#include "address.h"
#include "packet.h"

#include <chrono>
#include <atomic>

/** The states of sending the packets. */
enum SendPacketsState {
	SPS_CLOSED,      ///< The connection got closed.
	SPS_NONE_SENT,   ///< The buffer is still full, so no (parts of) packets could be sent.
	SPS_PARTLY_SENT, ///< The packets are partly sent; there are more packets to be sent in the queue.
	SPS_ALL_SENT,    ///< All packets in the queue are sent.
};

/** Base socket handler for all TCP sockets */
class NetworkTCPSocketHandler : public NetworkSocketHandler {
private:
	Packet *packet_queue;     ///< Packets that are awaiting delivery
	Packet *packet_recv;      ///< Partially received packet
public:
	SOCKET sock;              ///< The socket currently connected to
	bool writable;            ///< Can we write to this socket?

	/**
	 * Whether this socket is currently bound to a socket.
	 * @return true when the socket is bound, false otherwise
	 */
	bool IsConnected() const { return this->sock != INVALID_SOCKET; }

	NetworkRecvStatus CloseConnection(bool error = true) override;
	virtual void SendPacket(Packet *packet);
	SendPacketsState SendPackets(bool closing_down = false);

	virtual Packet *ReceivePacket();

	bool CanSendReceive();

	/**
	 * Whether there is something pending in the send queue.
	 * @return true when something is pending in the send queue.
	 */
	bool HasSendQueue() { return this->packet_queue != nullptr; }

	NetworkTCPSocketHandler(SOCKET s = INVALID_SOCKET);
	~NetworkTCPSocketHandler();
};

/**
 * "Helper" class for creating TCP connections in a non-blocking manner
 */
class TCPConnecter {
private:
	addrinfo *ai = nullptr;                             ///< getaddrinfo() allocated linked-list of resolved addresses.
	std::vector<addrinfo *> addresses;                  ///< Addresses we can connect to.
	size_t current_address = 0;                         ///< Current index in addresses we are trying.

	std::vector<SOCKET> sockets;                        ///< Pending connect() attempts.
	std::chrono::steady_clock::time_point last_attempt; ///< Time we last tried to connect.

	std::atomic<bool> is_resolved = false;              ///< Whether resolving is done.

	void Resolve();
	void OnResolved(addrinfo *ai);
	bool TryNextAddress();
	void Connect(addrinfo *address);
	bool CheckActivity();

	static void ResolveThunk(TCPConnecter *connecter);

public:
	std::string connection_string;                      ///< Current address we are connecting to (before resolving).

	TCPConnecter(const std::string &connection_string, uint16 default_port);
	virtual ~TCPConnecter();

	/**
	 * Callback when the connection succeeded.
	 * @param s the socket that we opened
	 */
	virtual void OnConnect(SOCKET s) {}

	/**
	 * Callback for when the connection attempt failed.
	 */
	virtual void OnFailure() {}

	static void CheckCallbacks();
	static void KillAll();
};

#endif /* NETWORK_CORE_TCP_H */
