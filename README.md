# Arduino-Nano-RPM-Speedo

I have put some Arduino PCBs to manufacture according to my protoboard. I have made a 12V to 5V converter and used transistors to transmitt signal.
On RPM pins i put one pin to GND and the second one i output signal with a twist. Like Ron suggested i use inline ceramic 105 cap. This creates pulses that go below the GND plane and my RPM indicator reads them as reluctance wheel signals.

There will also be some signal inputs on the board. For example ENABLE signal for the RPM to start with engine start not at the turn of the key. Now i get some noises when RPM dial starts to work even before i apply engine on signal from alternator. Maybe it just confuses the BSM.

Also i will have two additional signal outputs with transistor to signal 12V (pullup or open collector) if i need to start a cooling fan relay or something.

Code is in the Speedo ino file. One part of the code i use to generate speedo signal according to the RPM of the drive shaft collar with two magnets. i use magnetic hall sensor to send signal to Arduino.
The second part of the code generates RPM signal of 58pulses per rotation with 2 missing pulses that signaify end of turn. I use 850RPM idle so that my EV thinks engine is turning. 
