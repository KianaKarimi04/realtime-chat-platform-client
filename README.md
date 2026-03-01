# Distributed Chat System Client (C)

A C-based TCP client built as part of a distributed multi-server chat system with server-manager coordination and user authentication.

---

## Overview

This project implements the client component of a distributed chat architecture consisting of:

- A Server Manager
- Multiple backend servers
- Multiple TCP clients

The client connects to a central Server Manager, which coordinates communication with backend servers. Users can create accounts, log in, log out, and interact with the distributed system through structured message protocols.

The system demonstrates distributed architecture design, TCP socket communication, authentication handling, and multi-server coordination.

---

## Current Features

- TCP client-server communication using POSIX sockets
- Connection to Server Manager for routing
- Account creation and authentication (login/logout)
- Structured message protocol between client and servers
- Terminal-based interface

---

## In Progress

- Default shared chatroom implementation
- Real-time message broadcasting across connected clients
- Chat session management improvements

Note: Previously sent messages are not persisted yet (no message history storage).

---

## Technologies Used

- C (POSIX socket API)
- TCP/IP networking
- IPv4 socket programming
- Custom application-level messaging protocol
- Distributed system architecture concepts

---

## Running Locally

1. Compile client:

   gcc client.c -o client  

2. Run client:

   ./client <server_manager_ip> <port>

Example:

   ./client 127.0.0.1 9000

(Note: Requires Server Manager and backend servers running separately.)

---

## My Contributions

This project was developed collaboratively. My responsibilities included:

- Implementing client-side networking logic
- Designing message protocol structures
- Handling authentication workflows (account creation/login/logout)
- Debugging routing through Server Manager
- Organizing modular client code

---

## What I Learned

This project strengthened my understanding of:

- Distributed system design
- TCP socket lifecycle management
- Client-server coordination
- Authentication workflows
- Debugging multi-component networked systems

---

## Project Status

This project is under active development with additional features being implemented to expand chat functionality.
