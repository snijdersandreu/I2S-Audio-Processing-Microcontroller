# I2S-Audio-Processing-Microcontroller
In this project i use the ESP32 to perform a real-time spectrum analyzer. It processes audio signals from an ADC/DAC I2S module, visualizing frequency bands via an interactive web interface.

The motivation behind this project is to control a public fountain with music, allowing water jets and lights to respond dynamically to each frequency band.

The final code of this project is [i2sFFTWebpage](src/i2sFFTWebpage.cpp). In this report I'll try to explain the main concepts of it, but there will be minor improvements that I won't refer to.

## Table of Contents
- [Motivation](#motivation)
- [Features](#features)
- [Hardware](#hardware)
    - [Components](#components)
    - [Wiring](#wiring)
- [Software](#software)
    - [I2S fetching](#i2s-fetching)
    - [Windowing](#windowing)
    - [FFT](#fft)
    - [Bands and scaling of the values](#bands-and-scaling-of-the-values)
    - [Webpage](#webpage)
        - [Base Webpage](#base-webpage)
        - [Refreshing Values](#refreshing-values)
- [Usage and Performance](#usage-and-performance)
- [Acknowledgments](#acknowledgments)

## Motivation
The desire to merge technology with art and create a visually and auditorily dynamic public space drove the development of this project. The idea of making water jets and lights in a fountain react to music in real-time presents an exciting challenge and a beautiful way to interact with sound in a physical environment.

## Features
- Real-time audio spectrum analysis using FFT.
- Dynamic visual and physical response through a web interface.
- Control over a public fountain's water jets and lights based on the music's frequency bands.

## Hardware

### Components
List the components used in this project, including the ESP32 board and the specific ADC/DAC I2S module, along with any other sensors, lights, or actuators involved in controlling the fountain.

### Wiring
Provide a wiring diagram or a detailed description of how the components are connected.

## Software

### I2S Fetching
Explain how audio data is captured from the ADC/DAC I2S module using the ESP32.

### Windowing
Describe the windowing technique applied to the audio data before FFT processing to reduce spectral leakage.

### FFT
Detail the FFT process used to analyze the audio signal's frequency content.

### Bands and scaling of the values
Discuss how the FFT results are divided into frequency bands and how these bands correspond to different elements of the fountain.

### Webpage

#### Base Webpage
Introduce the web interface's structure and functionality, highlighting how it visualizes the audio spectrum.

#### Refreshing Values
Explain the method used to update the webpage with real-time data, ensuring the visualization remains dynamic.

## Usage and Performance
Share how to operate the system and any performance metrics or observations noted during its use. Discuss its responsiveness, accuracy in frequency analysis, and overall impact on the fountain's interactivity.

## Acknowledgments
Extend thanks to individuals, communities, or resources that significantly contributed to the project's success.
