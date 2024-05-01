# Robotic Arm Program

## Overview
This repository contains several robotic arm programs. Each program contains two key components: a Python script for color and shape tracking and an Arduino script for managing vision feedback based on color or shape detection. The Python script utilizes image processing libraries to detect and track different colors in real-time and send commands via UART. The Arduino script is designed to receive these commands and adjust actuators or feedback mechanisms accordingly.

## Project Structure
- `Tangram`: a program that enables robotic arm to solve Tangram puzzles
  - `color_tracking_feedback_location.py`: Python script for real-time color detection and tracking.
  - `Vision.ino`: Arduino sketch for receiving color tracking data and managing actuators or other hardware based on the received commands.
- `Angle Adjustment (pause)`: a program that adjusts the block angle on the conveyor.
  - `color_angle_adjust_dynamic`: Python script
  - `Vision.ino`: Arduino sketch
- `Angle Adjustment (continous)`: a program that adjusts the block angle on the conveyor.
  - `color_angle_adjust_dynamic_continuity`: Python script
  - `Vision.ino`: Arduino sketch
- `Color & Shape Classification`: a program that classifies blocks based on their colors and shapes.
  - `mul_color`: Python script
  - `Vision.ino`: Arduino sketch
- `Blocks Targeting`: a program that enables robotic arm to target specific blocks.
  - `color_tracking_feedback_location`: Python script
  - `Vision.ino`: Arduino sketch

## Python script

### Description
This script uses OpenMV libraries (`sensor`, `image`, `time`, `math`) for real-time image processing. It identifies different colors based on predefined thresholds and sends specific coordinates and adjustment commands through UART to an external controller (e.g., an Arduino).

### Features
- Real-time color detection.
- UART communication for sending real-time feedback.
- Detailed logging of detected colors and their positions for easy debugging and calibration.

### Setup and Dependencies
- OpenMV IDE for running and deploying the Python script.
- Pyb module for board-specific features like UART and LEDs.

## Vision System (Arduino)

### Description
The Arduino sketch (`Vision.ino`) receives data from the Python script and uses it to adjust mechanisms based on the color and location data received. This could involve adjusting angles, positions, or triggering other mechanical actions.

### Features
- Receives and parses UART data.
- Handles multiple color data inputs for versatile mechanism control.
- Provides robust error handling and feedback for system status.

### Setup and Dependencies
- Arduino IDE for editing, compiling, and uploading the sketch.
- Appropriate libraries and hardware setup for handling UART communication.

## Usage

### Running the Python Script
1. Open `Python script` with OpenMV IDE.
2. Connect your OpenMV cam or compatible device.
3. Run the script and observe the output through serial monitor or connected Arduino.

### Uploading and Running the Arduino Sketch
1. Open `Vision.ino` in the Arduino IDE.
2. Connect your Arduino board via USB.
3. Compile and upload the sketch to the Arduino.
4. Ensure it is properly connected to receive UART signals from the OpenMV cam or other sources.

## Contributing
Feel free to fork this repository, make improvements, and submit pull requests. For major changes or enhancements, please open an issue first to discuss what you would like to change.

## Acknowledgments
- Thanks to [WLKATA](https://www.wlkata.com/)
- Thanks to the developers of OpenMV and Arduino for providing the platforms used in this project.
- Special thanks to all contributors of the Python and Arduino communities.

