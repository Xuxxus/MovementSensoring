# Movement Sensoring

## Overview

This project is designed to read data from multiple MPU6050 IMUs and store the collected values on an SD Card using the ESP32 DEV KIT and Arduino IDE (version 1.8.19).
This is part of a OpenSenseRT based project. More details on the [main repository](https://github.com/Xuxxus/CodigoKalman) and on the [other repos](#Additional-Repositories) .

### Features

- **SimpleLoop Tag**: The first version of this project, using a single CPU core to process data. The code is simpler but may run slower.

- **RTOS-based Tag**: A different version that utilizes one CPU core for reading sensor data and the other for writing to the SD card. This configuration enhances performance, providing a faster frequency.

## Getting Started

### Prerequisites

- Arduino IDE 1.18.19
- ESP32 DEV KIT
- MPU6050 IMUs
- SD Card Module

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/your-username/your-repo.git

2. Open the Arduino IDE and load the project.

3. Configure the Arduino IDE for ESP32 DEV KIT and install any required libraries.

4. Connect the MPU6050 IMUs and SD Card Module to the ESP32 DEV KIT following the provided pin configurations.

### Usage

1. Choose the appropriate tag (SimpleLoop or RTOS-based) in the Arduino IDE based on your requirements.

2. Upload the code to the ESP32 DEV KIT.

3. Monitor the serial console for debugging information.

## Contributing

If you want to contribute to this project, follow these steps:

1. Fork the repository.

2. Create a new branch for your feature:
   
   ```bash
    git branch feature-name
   ```
    ```bash
    git checkout feature-name

4. Make your changes and commit them:
   ```bash
   git add .
   ```
    ```bash
    git commit -m 'Add your feature'

6. Push to the branch:

    ```bash
    git push origin feature-name

7. Create a pull request. Please, be clear about what you did.

## Additional Repositories

* [OpenSense Data Processing](https://github.com/Xuxxus/CodigoKalman): Main repository for processing data suitable for OpenSense software.

* [PCB Repository](https://github.com/Xuxxus/ConnectionBoard): Repository for the PCB used in this project.

## Acknowledgments

Professors Wellington Pinheiro, MSc. and Maria Claudia Castro, PhD. from FEI University Center for their absolute great job on guiding our project. 
