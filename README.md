# vpn_status

Simple program to monitor whether a VPN connection is active. This is done by monitoring the interfaces and checking whether any tap or tun devices exist.  

---

#### Current state

If a tap or tun interface is added (this happens when a VPN connection is established), the program will execute a user-defined command.
The same is done whenever a tap or tun device is removed - a VPN connection was severed. In this case, another user-defined command is called.

These *user-defined commands* may be any call that would work in a shell. The path to a script, some file in your path, anything. 

:exclamation: **Be careful** The provided commands are not checked in any way before calling them.

---


### Usage

| cmd         |action |
|-------------|-------|
|-f \<path>   | route all logging to file at *path* 
|-q           | route all logging to file */tmp/vpn_status.log*
|-v \<level>  | set loglevel (0...4) [default 3]
|-u \<command>| set command to call when vpn up
|-d \<command>| set command to call when vpn down
|-h           | print help


### Examples

    # start vpn_status in the background, don't perform any action
    # and log to file /tmp/vpn_status.log
    vpn_status -q&
    
    # call vpn_up.sh when a vpn connection is established
    # and vpn_down.sh when it is severed
    vpn_status -u ~/scripts/vpn_up.sh -d ~/scripts/vpn_down.sh -q&
    
    # restart finicky applications
    vpn_status -u "killall teams; teams&" -d "killall teams; teams&" -q&


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


