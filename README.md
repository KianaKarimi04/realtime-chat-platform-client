# Real-Time Chat Platform (C)

A collaborative client-server chat application built in C using TCP sockets.

---

## Overview

This project implements a real-time chat client and server in C using socket programming.  
Multiple clients can connect to the server and send messages that are visible to all connected participants.

The system demonstrates low-level networking concepts, client-server communication over TCP/IP, and basic protocol handling.

---

## Features

- Client-server architecture using TCP sockets  
- Real-time message broadcast to all connected clients  
- Basic UI interaction in the terminal  
- Concurrent client handling using process/thread management

---

## Technologies Used

- C (POSIX socket API)
- TCP/IP networking
- Terminal-based interaction
- IPv4 socket programming
- Basic application-level messaging protocol

---

## Running Locally

1. Compile server and client:

   gcc server.c -o server  
   gcc client.c -o client  

2. Start the server:

   ./server  

3. Connect each client:

   ./client <server_ip> <port>

Example:

   ./server  
   ./client 127.0.0.1 9000

---

## My Contributions

This project was completed collaboratively with a team member. My work included:

- Implementing the client networking logic  
- Designing communication protocol structure  
- Debugging concurrent message handling  
- Organizing modular code with header files

---

## What I Learned

This project strengthened my understanding of socket programming, TCP/IP communication, and building client-server systems in C.

---

## Project Status

This project is actively being improved. Additional features and refinements are currently in development.
