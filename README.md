# MassHunter Middleware Service

A C++ middleware that automates measurement-instrument software (Agilent MassHunter) and integrates it with process-control systems. The middleware implements UI automation, TCP/IP services, XML-based command handling and real-time status monitoring to enable fully automated measurements, method selection, error reporting and data-path management.

---

## Table of Contents

- Overview
- Key Features
- Architecture & Important Source Files
- Build Requirements
- Configuration
  
---

## Overview

This project is a Windows desktop/service middleware component that connects to MassHunter (instrument software) to:
- read projects and methods,
- start runs and run-sequences,
- monitor instrument and run status in real time,
- expose web/TCP endpoints for upstream systems (LIMS / SAMI) to query and start methods,
- automatically reply to dialogs and manage UI interactions where necessary.

---

## Key Features

- UI automation to control the MassHunter GUI and detect its state.
- TCP/IP-based services:
  - SAMI service (default port 9002)
- XML-based command and data structures for run scheduling (classes such as CExecutiveParameters, CMethodRun, CMethodSchedule).
- Real-time status monitoring (instrument & run status).
- Automatic reply-to-dialogs option to handle common MassHunter popups.
- Registry-based configuration and persistent UI settings.
- Logging to a shared Documents folder.

---

## Architecture & Important Source Files

1. **Measurement software Interface**
   
  - This part of the program manages the communication to the instrument side.  
  - Part of the task is to read and send status information and sample parameters.
  - Core interface classes for communicating with MassHunter and representing run metadata (CExecutiveParameters, CMethodRun, CMethodSchedule, CMassHunterInterface, ...).
  - Handles interaction with MassHunter UI, COM integration (optional), and job tracking.
    
2. **Middleware main program**
   
   This code manages the communication with the process management software to send and receive messages in XML format.  
   Moreover, it contains the GUI code of the middleware implemented in the MFC framework.
   
  - Main dialog/service code that:
    - hosts the GUI / taskbar integration,
    - initializes MassHunter interface,
    - starts/stops the and process control managment software,
    - dispatches run requests and handles run lifecycle,
    - provides configuration, logging and status handling.
      
3. **Test program:**
   
   There are in this folder the codes of the test program, which implement two types of tests:

   - **Service client test:**  
     Through this test, the automated processes of the instrument software and communication via the appropriate port number and IP address with process management software      were proven. Furthermore, the test verified the sending of the error message to the server.

   - **Service command test:**  
     The test program checks in all command classes whether the received command and processed XML correspond to the sent command or not.

5. **Command handler:**
   
   This part of the program conducts the commands to run a task in the measurement instrument.  
   It receives, converts, saves, and sends the messages in XML format.  
   Examples of tasks include starting or stopping a measurement, receiving error or status information, and sending sample parameters.
---

## Build Requirements

- Windows (development & target)
- Visual Studio (recommended 2015/2017/2019/2022) with:
  - MFC (use shared MFC DLL or static depending on your setup)
  - C++ compiler and Windows SDK
- Linker libraries used in source:
  - user32.lib
  - shell32.lib
  - ws2_32.lib
  - Gdiplus.lib
  - Ole32.lib (for COM)
---
