This gpsserver app is designed to receive data from a Xexun TK102-2 tracker over GPRS connection. It parses the data to a format compatible with gpsseuranta.net data. This app is largely based on tutorial in http://www.tidytutorials.com/2010/06/linux-c-socket-example-with-client.html. You can compile this app with g++, for example "g++ -o gpsserver gpsserver.cpp -lpthread" with source file name changed. Feel free to do what ever you like with this piece of code, just include my name with it if you share it. You can also contact me via email if you like.

Usage
You should first modify the parameters in the code to meet your setup. These are server port, data.lst location and SendTo parameters host ip in few places and the data string to be sent. You can just comment out the SendTo-line if you wish not to relay the data.
Next thing is to fill the 'imeis.txt' with each line consisting of ' imei:{your tracker imei here}' and '{your id for the tracker}' combined with a comma. For example: ' imei:16423156251,tracker01' without the apostrophes.
What you do next is that you compile the source into a executable like explaned above. Next you can start the server, preferably under screen ('screen -S gpsserver', for example) so you can detach it (Ctrl-A, D) and leave it running.
Next you can set your tracker to connect to your server ip and selected port. (You can find out how by browsing your manual or the Internet.)
To send back commands you can, just while gpsserver is running, modify the commands.txt with a desired command, such as 'sdlog123456 1' or similar. I think you can only send one command at a time. You can see while running the gpsserver, when the command is sent, and you can read the reply from gps.log.
One thing, that's not required but quite interesting, is that if you are running a webserver on the same server, you can make a symbolic link of the gps.log to some textfile under your webserver document tree, so you can monitor the incoming data by accessing this textfile. ('ln -s /home/heikki/gpsserver/gps.log /home/www/gpslog.txt', for example)
I have also included a bash-script (gpslog.sh) I use to keep my log tidy. You just change the folders right, and run the script for example via crontab, and it will copy the log to a another folder with the current timestamp as filename. It will also empty the gps.log. I have used an interval of an hour which works quite well.

Copyright 2012 Heikki Virekunnas
heikki@virekunnas.fi
http://heikki.virekunnas.fi