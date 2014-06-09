I2C Ethernet Firmware README
============================

Overview
--------
The I2C Ethernet module by Open Source Control Systems is a small module that is used to relay data requests and commands between web clients and an embedded device. The I2C Ethernet module acts as an I2C Slave device. Web requests to a specific URL are buffered and made available to the application when polled. The application returns a response to the I2C Ethernet module which is then returned as the response of the web request.

I2C Ethernet supports GET requests to /btnic.cgi. The querystring of the GET request is translated to a simple ASCII format during the buffering process:

  * Each request is prefaced with a "start of request" character (0x01) (As of I2CETH v2.0)
  * Each ampersand (&) character delimiting querystring variables is translated to a tab '\t' character
  * Requests are terminated with a carriage return '\r' character

Responses are returned to the module from the application as an ASCII string in a similar format:

  * Response fields are tab '/t' delimited
  * Responses are terminated with a carriage return '\r' followed by a new line '\n'

Responses are returned to the web client as a simple JSON array:

  * Example: ["field1","field2","field3"]

Firmware Updates
----------------
Firmware updates to the I2C Ethernet module can be uploaded using the TFTP protocol. 

  * Modules with a firmware v2.2 or older must be physically reset prior to starting the upload and must be uploaded to the bootloader's static IP of 192.168.97.60.
  * Modules with firmware v2.5 or newer can upload the new firmware directly to the module's current IP address. The module will automatically reset. If necessary, a hardware reset is still supported using the same static IP of 192.168.97.60.

Web Content
-----------
Modules with firmware prior to v2.5 include a small file system compiled into the main program flash. No additional upload is required to populate web content.

Modules with firmware v2.5 and newer now support user upload of custom web content images. Web content can be uploaded using a built-in URL even if no existing web content has been uploaded. A system without web content will report a 404 error. To upload web content browse to /mpfsupload and upload the MPFS image using the form displayed.

A standard web content image is included with the firmware source code as MPFSImg2.bin. Users can choose to generate their own custom images using the Microchip MPFS utility for Windows "MPFS2.exe" or using a the Java application "MPFS.jar".

When creating a custom web content image make sure you include the following files to support data requests, system configuration and diagnostics:

  * btnic.cgi
  * crossdomain.xml
  * index.html
  * protect/
    * btcom.htm
    * config.htm
    * eeprom.cgi
    * flash.cgi
    * index.html
    * mchp.js
    * reboot.cgi
    * reboot.htm
    * sram.cgi
    * webconf.htm
