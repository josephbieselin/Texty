# Texty

Programmer: Joseph Bieselin

Twitter Clone using HTML, PHP, C++
Currently no CSS styling.
"Database" is emulated on localhost using C++ file handling code.
C++ networking code implemented on localhost with an arbitrary port number.
Distributed network will be used to replicate and run multi-threaded C++ code to handle multiple requests later on.



Assumptions:
-Machine is configured with a LAMP server to access localhost in a web browser
-The index file for the web app is located on a Unix machine in the path "/var/www/html"
-Write and executable access is needed on the html directory for anyone because the localhost web app will be creating and removing directories/files
-Executable access is needed for anyone along the path "/var/www/html" because the web app needs to know the path of where the files are



TO RUN WEB APP:
1) Move all the files into the "/var/www/html" directory
2) Compile the texty.cpp file
3) Run the compiled file
4) The socket will now be opened and waiting for a connection from localhost on a web browser
5) Open a one of your probably two favorite web browsers of choosing
6) Type enter "localhost" in the address bar and press enter
7) Enjoy the experience