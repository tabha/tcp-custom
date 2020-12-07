

Demo
----

Features
---------
- tcp connection procedure
- loss simulation
- loss recovery management
- packet buffering over IP
- Adaptation of the RRT
- STOP AND WAIT

**Featured quote**

**Versions**
--------------


**Version 1**
-------------

-the first version implements a no-loss recovery version. Thus any packet lost during the transmission phase will not be retransmitted.

**Version 2**
---------------
-In version two, we are interested in an implementation that guarantees total reliability via a "Stop and Wait" type loss recovery mechanism.


**version 3**
------------
-a guarantee of "static" partial reliability via a "Stop and Wait" type loss recovery mechanism with "pre-wired" partial reliability, i.e. with a statically defined % of admissible losses


**Version 4**
--------------
-a connection establishment phase
-a guarantee of partial reliability through a "Stop and Wait" loss recovery mechanism, the % of allowable losses of which is now negotiated during the connection establishment phase


**Final Version**
-----------------
-Connection phase (since v4).

-Negotiation of the percentage of loss allowed (this negotiation is added in the header during the connection phase).

-Calculation of the RTT and adaptation of the value during communication (the maximum value is set here to 1ms as we are in local host).
   (at home with the ping localhost I have a rtt close to 0.43ms)
   in real communication will have to change the max. value and adapt to the network specification.
