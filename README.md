# vpn_status

Simple program to monitor whether a VPN connection is active. This is done by monitoring the interfaces and checking whether any tap or tun devices exist.  

---------

#### Current state

If a tap or tun interface is added (this usually happens when a VPN connection is established), the program will write a symbol to a file `$HOME/.vpn_status`.