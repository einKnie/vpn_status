# vpn_status

Simple program to monitor whether a VPN connection is active. This is done by monitoring the interfaces and checking whether any tap or tun devices exist.  

---

#### Current state

If a tap or tun interface is added (this happens when a VPN connection is established), the program will write a symbol to a file `$HOME/.vpn_status`.  
The reverse is done whenever a tap or tun device is removed - a VPN connection was severed. I this case, the symbol is removed from the file.

This implementation is for personal use, but some sort of user-configurable action instead of the file write is planned for the future.

---

The program does not daemonize itself but is best run in the background, as all log output is written to a file /tmp/vpn_status.log regardless.