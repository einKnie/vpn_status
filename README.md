# vpn_status

Simple program to monitor whether a VPN connection is active. This is done by monitoring the interfaces and checking whether any tap or tun devices exist.  

---

#### Current state

If a tap or tun interface is added (this happens when a VPN connection is established), the program will write a symbol to a file `$HOME/.vpn_status`.  
The reverse is done whenever a tap or tun device is removed - a VPN connection was severed. I this case, the symbol is removed from the file.

This implementation is for personal use, but some sort of user-configurable action instead of the file write is planned for the future.

---

The program does not daemonize itself but is best run in the background, as all log output is written to a file /tmp/vpn_status.log regardless.


### Usage

| cmd        |action |
|------------|-------|
|-f \<path\> | route all logging to file at *path* 
|-d          | route all logging to file */tmp/vpn_status.log*
|-v \<level\>| set loglevel (0...4) [default 3]
|-h          | print help


### Examples

    vpn_status -d&   # start vpn_status in the background and log to file /tmp/vpn_status.log


#### Example logfile

    
    (PID 20826) | 22:48:19 |  NOTICE  | Got current interface data:
	(PID 20826) | 22:48:19 |  NOTICE  | o | lo          : 00:00:00:00:00:00
	(PID 20826) | 22:48:19 |  NOTICE  | o | enp12s1     : 3c:49:37:18:f5:44
	(PID 20826) | 22:48:19 |  NOTICE  | x | enp14s0     : 00:25:22:cf:70:ac
	(PID 20826) | 22:48:19 |  NOTICE  | x | virbr0      : 52:54:00:27:84:1f
	(PID 20826) | 22:48:19 |  NOTICE  | x | virbr0-nic  : 52:54:00:27:84:1f
	(PID 20826) | 22:50:22 |  NOTICE  | 
	--
	A VPN interface was added
	--
	(PID 20826) | 22:50:22 |  NOTICE  | Interface added:
	(PID 20826) | 22:50:22 |  NOTICE  | tap0        : d6:20:e3:d1:d1:83
	(PID 20826) | 22:50:22 |  NOTICE  | New state:
	(PID 20826) | 22:50:22 |  NOTICE  | o | lo          : 00:00:00:00:00:00
	(PID 20826) | 22:50:22 |  NOTICE  | o | enp12s1     : 3c:49:37:18:f5:44
	(PID 20826) | 22:50:22 |  NOTICE  | x | enp14s0     : 00:25:22:cf:70:ac
	(PID 20826) | 22:50:22 |  NOTICE  | x | virbr0      : 52:54:00:27:84:1f
	(PID 20826) | 22:50:22 |  NOTICE  | x | virbr0-nic  : 52:54:00:27:84:1f
	(PID 20826) | 22:50:22 |  NOTICE  | x | tap0        : d6:20:e3:d1:d1:83
	(PID 20826) | 22:50:22 |  NOTICE  | Changes detected on interface tap0:d6:20:e3:d1:d1:83 (x)
	(PID 20826) | 22:50:22 |  NOTICE  | Changes detected on interface tap0:d6:20:e3:d1:d1:83 (x)
	(PID 20826) | 22:50:22 |  NOTICE  | Changes detected on interface tap0:d6:20:e3:d1:d1:83 (o)
	(PID 20826) | 22:50:22 |  NOTICE  | New state:
	(PID 20826) | 22:50:22 |  NOTICE  | o | lo          : 00:00:00:00:00:00
	(PID 20826) | 22:50:22 |  NOTICE  | o | enp12s1     : 3c:49:37:18:f5:44
	(PID 20826) | 22:50:22 |  NOTICE  | x | enp14s0     : 00:25:22:cf:70:ac
	(PID 20826) | 22:50:22 |  NOTICE  | x | virbr0      : 52:54:00:27:84:1f
	(PID 20826) | 22:50:22 |  NOTICE  | x | virbr0-nic  : 52:54:00:27:84:1f
	(PID 20826) | 22:50:22 |  NOTICE  | o | tap0        : d6:20:e3:d1:d1:83


