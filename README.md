# Robot Gold Collection Game

## Overview
This project implements a simulation where a robot moves on a grid to collect gold pieces while avoiding bombs. The game runs on a multi-threaded architecture using the C++ Standard Library's threading capabilities. It is a simple command line game that demonstrates thread synchronization.

## Features
- **Multi-threaded Execution:** Utilizes multiple threads to simulate simultaneous movements of the robot and potentially other elements like bombs.
- **Automated Movements:** The robot and other elements on the board move according to predefined rules and random generation.

## Requirements
- C++17 or higher
- Compiler with support for the C++ Standard Library and multi-threading (e.g., g++, clang)

## Compilation and Running
1. **Compilation:**
   Use the following command to compile the game:
   ```bash
   g++ -std=c++17 -pthread RobotGold.cpp -o RobotGold
   ```
2. **Running:**
   Run the compiled executable using:
   ```bash
   ./RobotGold
   ```

## Game Rules
- The robot starts at a random position on the grid.
- Each turn, the robot moves to adjacent cells trying to collect gold while avoiding bombs.
- The game ends when all gold is collected or the robot encounters a bomb.
