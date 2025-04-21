# Congestion Control for Pseudo Real-Time Audio Streaming

This project implements and evaluates a UDP-based pseudo real-time audio streaming application featuring feedback congestion control. The primary goal is to manage network congestion effectively while streaming raw-like audio data (.au format) between a server and a client.

## Overview

The application consists of a server (`audiostreams`) and a client (`audiostreamc`). The client requests a specific `.au` audio file from the server. The server then streams the audio data over UDP to the client. The client plays the received audio using the ALSA library and provides feedback to the server about its buffer status. The server uses this feedback to dynamically adjust its sending rate using different congestion control algorithms.

## Features

* **UDP-based Audio Streaming:** Utilizes UDP for transmitting audio data packets.
* **Client-Server Architecture:** A concurrent server handles multiple client requests by forking child processes for each streaming session.
* **Pseudo Real-Time Playback:** Client plays `.au` audio files using ALSA (`mulawopen`, `mulawwrite`) with a fixed playback interval (313ms).
* **Feedback Congestion Control:**
    * **Smart Sender/Dumb Receiver:** Client sends buffer occupancy feedback (0-20 scale, 10 = target) to the server.
    * **Control Methods:** Server implements congestion control methods C and D (selectable via `CONTROLLAW` macro) to adjust sending rate (`lambda`, controlled via `packetinterval`).
    * **Configurable Parameters:** Control parameters `epsilon` and `gamma` (for Method D), and initial sending rate `lambda` are configurable.
* **Buffer Management:** Client uses a FIFO buffer (producer-consumer model) with semaphore protection to store incoming audio data before playback.
* **Performance Logging:**
    * Server logs timestamped sending rate (`packetinterval`) changes.
    * Client logs timestamped buffer occupancy (`Q(t)`) changes.
    * Logs are normalized to start time 0 and saved to separate files per session/client PID.
* **Configurable Streaming Parameters:** Filename (max 20 chars), `blocksize` (payload size), `buffersize` (client buffer capacity), and `targetbuf` (target buffer occupancy) are configurable via command-line arguments.
* **Session Termination:** Server sends 5 empty UDP packets to signal the end of the stream. Client has a 2-second timeout for the initial server response.
* **(Bonus) Method H:** Includes an optional, custom congestion control method (Method H), selectable via `CONTROLLAW=5`.

## Architecture

1.  **Client Request:** `audiostreamc` sends a UDP request to `audiostreams` containing the `audiofile` name and desired `blocksize`.
2.  **Server Fork:** The main `audiostreams` process forks a child process to handle the request on a new ephemeral UDP port.
3.  **Streaming:** The child server process reads the audio file and sends data packets to the client at a rate controlled by `packetinterval`.
4.  **Client Buffering & Playback:** `audiostreamc` receives packets, stores them in its FIFO buffer (Producer), and periodically reads data from the buffer to play via ALSA (Consumer).
5.  **Feedback:** After buffer read/write operations, the client calculates its buffer state relative to `targetbuf` and sends a feedback value (0-20) back to the child server process.
6.  **Server Rate Adjustment:** The child server process receives feedback and adjusts `packetinterval` according to the selected congestion control method (C or D).
7.  **Logging:** Both client and server log relevant metrics (buffer occupancy, sending interval) with timestamps.
8.  **Termination:** Server signals end-of-stream; child process writes logs and exits. Client writes logs and exits.

## Build Instructions

1.  **Prerequisites:** Ensure you have `gcc` (or a compatible C compiler) and the ALSA development library installed (e.g., `libasound2-dev` on Debian/Ubuntu).
    ```bash
    # Example for Debian/Ubuntu
    sudo apt-get update
    sudo apt-get install build-essential libasound2-dev
    ```
2.  **Compile:** Navigate to the project directory containing the source code and `Makefile`. Run `make`:
    ```bash
    make
    ```
    This will compile `audiostreamc` and `audiostreams`, linking against the ALSA library (`-lasound`).

## Usage

**Server:**

```bash
./audiostreams <lambda> <epsilon> <gamma> <logfileS> <server-IP> <server-port>
