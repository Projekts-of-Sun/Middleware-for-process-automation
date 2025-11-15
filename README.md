This program consists of four parts:

1. **Test program:**  
   There are in this folder the codes of the test program, which implement two types of tests:

   - **Service client test:**  
     Through this test, the automated processes of the instrument software and communication via the appropriate port number and IP address with process management software were proven. Furthermore, the test verified the sending of the error message to the server.

   - **Service command test:**  
     The test program checks in all command classes whether the received command and processed XML correspond to the sent command or not.

2. **Command handler:**  
   This part of the program conducts the commands to run a task in the measurement instrument.  
   It receives, converts, saves, and sends the messages in XML format.  
   Examples of tasks include starting or stopping a measurement, receiving error or status information, and sending sample parameters.

3. **Measurement software middleware:**  
   The middleware is an interface between the process management software and the software of the measurement instrument.  
   This part of the program manages the communication to the instrument side.  
   Part of the task is to read and send status information and sample parameters.

4. **Middleware main program:**  
   This code manages the communication with the process management software to send and receive messages in XML format.  
   Moreover, it contains the GUI code of the middleware implemented in the MFC framework.
