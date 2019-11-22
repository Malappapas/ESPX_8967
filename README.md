# Embedded-Systems-Exercise-2
School project to connect 2 raspberry pi and exchange new messages saved a circular buffer
* Didn't manage to take measurements due to deadlines and too lazy to check if right

# Explenation

Code makes the raspberry act as a server and then as client for a random time. 
- As a server, it takes numbers from a list of unique 4-digit codes, edits it to a wifi address (10.0.XX.XX) to be used, creates a custom message "SendersUniqueCode_ReceiversUniqueCode_TimeSent_HelloThere!", sends it and every message saved in server's circular buffer.
- As a client, the raspberry connects to port and waits for a connection. When one is established, it accepts every package and cross references it with its circular buffer. If its a new it's added to the buffer, if not its discarded and gets to the next one. 
