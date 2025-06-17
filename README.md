# # **[VCEnhancedKeyClear](https://github.com/EngineOwningSoftware/VCEnhancedKeyClear)** for [VeraCrypt](https://www.veracrypt.fr/en/Home.html)

While Direct Memory Access (DMA) attacks gained some [recognition in the game hacking scene](https://github.com/EngineOwningSoftware/pcileech-webradar) they were initially used for data extraction from encrypted computers. Tools exist to grab encryption keys for full disk encryption from RAM to fully recover drive contents if a DMA device can be inserted to the running system.


# Protection

The widely used TrueCrypt fork **VeraCrypt** is equipped with an option called *Clear encryption keys from memory if a new device is inserted* since version 1.24. While this is an efficient way to protect against key extraction via DMA it has some drawbacks:

 - The option has to be re-enabled manually after every computer restart
 - Whenever a new device is inserted while this option is enabled (USB sticks, monitors, etc) the system will hang and eventually bluescreen because Windows can't access system files anymore.


# Tradeoff

VCEnhancedKeyClear was developed to have a tradeoff between the mentioned drawbacks and the valueable protection of that mechanism. The small tool registers a notification when the computers lock-state changes using the WinAPI function *[WTSRegisterSessionNotification()](https://docs.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsregistersessionnotification)* and switches the *Clear encryption keys from memory if a new device is inserted* option on demand so it is only enabled while the system is locked.

Also a small installer is ~planned~ **implemented** (see below) so the tool can be automatically started when a user logs on.
Additionally it features a tray icon so you can disable it at any time.

# Update 0.2

Since VeraCrypt version 1.26.7 when you turn on *Clear encryption keys from memory if a new device is inserted*, it will build a whitelist of known devices which are allowed to reconnect. VCEnhancedKeyClear has been updated to notify the VeraCrypt service to rebuild the device list when you lock the station.
Therefore displays that wake up from sleep, network interruptions etc. do not cause system crashes anymore making this tool as usable as ever!
Also you can now run VCEnhancedKeyClear with the *--install* command line parameter (as admin!) which will copy itself to your *Program Files* folder and create a scheduled task to automatically launch VCEnhancedKeyClear on logon.
You can use *--uninstall* to remove the file and scheduled task again.
Please note: multi user support has not been tested nor is it expected to work without issues at this time.

# Download

A precompiled build of this software can be found at [Releases](https://github.com/EngineOwningSoftware/VCEnhancedKeyClear/releases).
