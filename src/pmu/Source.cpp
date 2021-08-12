/*
Copyright (c) 2021,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See
the top-level NOTICE for additional details. All rights reserved. SPDX-License-Identifier: BSD-3-Clause
*/

#include "Source.hpp"

#include <asio/io_context.hpp>
#include <asio/dispatch.hpp>
#include <asio/strand.hpp>

#include <iostream>

using tcp = asio::ip::tcp;  // from <asio/ip/tcp.hpp>


// LCOV_EXCL_START
// Report a failure
static void fail(asio::error_code ec, char const *what) { std::cerr << what << ": " << ec.message() << "\n"; }

// LCOV_EXCL_STOP
// 

static constexpr std::size_t bufferSize = 63336 * 2;

// 
class PmuSession : public std::enable_shared_from_this<PmuSession>
{
    tcp::socket mSocket;
    std::vector<std::uint8_t> mBuffer;
    asio::steady_timer mTimer{mSocket.get_executor()};

  public:
    // Take ownership of the socket
    explicit PmuSession(tcp::socket &&socket) : mSocket(std::move(socket)) 
    { 
        mBuffer.resize(bufferSize);
    }

    // Get on the correct executor
    void run()
    {
       
    }

    // Start the asynchronous operation
    void on_run()
    {
        
    }

    void on_accept(asio::error_code ec)
    {
        if (ec)
        {
            return fail(ec, "helics websocket accept");
        }

        // Read a message
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        mSocket.async_receive(asio::buffer(mBuffer.data(),mBuffer.size()), [this](asio::error_code ec, std::size_t bytes_transferred) {
            on_read(ec, bytes_transferred);
        });
    }

    void on_read(asio::error_code ec, std::size_t /* bytes_transferred*/)
    {

        mSocket.async_send(asio::buffer(mBuffer.data(),mBuffer.size()), [this](asio::error_code ec, std::size_t bytes_transferred) {
            on_write(ec, bytes_transferred);
        });
    }

    void on_write(asio::error_code ec, std::size_t bytes_transferred)
    {
        
        if (ec)
        {
            return fail(ec, "helics socket write");
        }

       
        // Do another read
        do_read();
    }
};

// 
// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
    asio::io_context &ioc;
    tcp::acceptor acceptor;
    bool websocket{false};

  public:
    Listener(asio::io_context &context, const tcp::endpoint &endpoint, bool webs = false)
        : ioc(context), acceptor(asio::make_strand(ioc)), websocket{webs}
    {
        asio::error_code ec;

        // Open the acceptor
        acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "helics acceptor open");
            return;
        }

        // Allow address reuse
        acceptor.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "helics acceptor set_option");
            return;
        }

        // Bind to the server address
        acceptor.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "helics acceptor bind");
            return;
        }

        // Start listening for connections
        acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "helics acceptor listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run() { do_accept(); }

  private:
    void do_accept()
    {
        acceptor.async_accept(asio::make_strand(ioc),
                               [this](const asio::error_code &error, tcp::socket peer) {
                                   on_accept(error, std::move(peer));
                               });
    }

    void on_accept(asio::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            return fail(ec, "helics accept connections");
        }
        else
        {
            // Create the session and run it
            std::make_shared<PmuSession>(std::move(socket))->run();
            
        }

        // Accept another connection
        do_accept();
    }
};

namespace pmu
{

	void TcpSource::startServer(asio::io_context &io_context)
{
		connection = helics::tcp::TcpConnection::create(io_context, interface, port, 65536);
		connection->waitUntilConnected(std::chrono::milliseconds(4000));
		buffer.resize(65536);
        return;
	}


    void TcpSource::loadConfig() {

    }
    }