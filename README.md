# # **[VCEnhancedKeyClear](https://github.com/EngineOwningSoftware/VCEnhancedKeyClear)** for [VeraCrypt](https://www.veracrypt.fr/en/Home.html)

While Direct Memory Access (DMA) attacks gained some [recognition in the game hacking scene](https://github.com/EngineOwningSoftware/pcileech-webradar) they were initially used for data extraction from encrypted computers. Tools exist to grab encryption keys for full disk encryption from RAM to fully recover drive contents if a DMA device can be inserted to the running system.


# Protection

The widely used TrueCrypt fork **VeraCrypt** is equipped with an option called *Clear encryption keys from memory if a new device is inserted* since version 1.24. While this is an efficient way to protect against key extraction via DMA it has some drawbacks:

 - The option has to be re-enabled manually after every computer restart
 - Whenever a new device is inserted while this option is enabled (USB sticks, monitors, etc) the system will hang and eventually bluescreen because Windows can't access system files anymore.


# Tradeoff

VCEnhancedKeyClear was developed to have a tradeoff between the mentioned drawbacks and the valueable protection of that mechanism. The small tool registers a notification when the computers lock-state changes using the WinAPI function *[WTSRegisterSessionNotification()](https://docs.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsregistersessionnotification)* and switches the *Clear encryption keys from memory if a new device is inserted* option on demand so it is only enabled while the system is locked.

Also a small installer is planned so the tool can be automatically started with the computer.
Additionally it features a tray icon so you can disable it at any time.

# Download

A precompiled build of this software can be found at [Releases](https://github.com/EngineOwningSoftware/VCEnhancedKeyClear/releases).
