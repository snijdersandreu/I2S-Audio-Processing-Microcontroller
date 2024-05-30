# I2S-Audio-Processing-Microcontroller
In this project I use the ESP32 to perform a real-time spectrum analyzer. It processes audio signals from an ADC/DAC I2S module, visualizing frequency bands via an interactive web interface.

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

