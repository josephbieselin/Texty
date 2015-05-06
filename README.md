Texty Web App
Twitter Clone

Programmer:
Joseph Bieselin

Networking code interfaces PHP on user machine with a server running C++ code.
"Database" emulated with C++ file handling.
Threading will be used to handle multiple requests.
Replication will be implemented as pseudo-servers running in localhost on separate ports.


1) This website was created and tested on an Ubuntu LAMP server with localhost path "/var/www/html/"
2) Writable access to the parent directory "html" is required to create directories and files to emulate running the website and "servers"
3) All directories give RWX access to everyone


Each replication manager has their own directory; the name is just a stingified int:
	RM1	-->	"1"
	RM2	-->	"2"
	RM3	--> "3"
	RMX	--> "X+1"

Goals set by different stages for replication part of the project:
	1) Simply handle all data having all data replicated on each "server"
		- i.e. the each replication manager will write data sent by PHP from a user
	2) Each replication manager updates its own data, then sends the serialized string to other RMs to update their own data
	3) Handle any of the replication managers dying (crashing or not responding for some reason)
		- a primary (leader) is not used in my design
		- any replication manager can go down, and the user will only notice a small lapse in load time if the client PHP code tried to connect to a down server (RM)
		- assumption is made that a replication manager does not rise from the dead while other replication managers are running
		- crashed replication managers will only come back up during server down time in which case files would be copied manually from a replication manager that did not crash
		- if all servers crash at once whlie running, mandatory server down time will be taken to copy files from the last replication manager that was running
	4) Creating a new replication manager
		- create another instance of an RMX.cpp file
		- in that file update the directory and file path constants to relate to the number of the X in RMX (since path names are used since this is run on localhost on a local machine)
		- in each RM.cpp file and the funcs_and_consts.php, increase the MAX_PORT constant to include this new RM
		- edit the THIS_PORT constant to be the new, unique port number added to the list of MIN_PORT to MAX_PORT ports

Structure of all_users.txt:
N,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,'\n'
username,email,index_i,password,firstname,lastname,,,,,,,,,,,,,,,,,,,,,,,,,,,,,'\n'
username,email,index_j,password,firstname,lastname,,,,,,,,,,,,,,,,,,,,,,,,,,,,,'\n'
username,email,index_k,password,firstname,lastname,,,,,,,,,,,,,,,,,,,,,,,,,,,,,'\n'

... continues on for however many users are in the file;
Where N is index of the next user (this can go to infinity since numbers are not reused after a user deactivates their account);
We assume for now that 10 bytes corresponding to the string "999999999" will suffice for 99999999 possible unique user indexes;
Each line contains strings representing user info separated by commas;
Each line is 118 bytes where commas are put at the end of each line before the newline character so lines can be overwritten after created


Structure of followers.txt & followees.txt:
index_i,username,,,,,,,,,,,,,,,,,,,,,,,,,,'\n'
index_j,username,,,,,,,,,,,,,,,,,,,,,,,,,,'\n'

... continues on for however many users are in the file;
The user that the person is followed by (follower) or the that is following (followee) has their index and username on a line;
Each line is 28 bytes where commas are put at the end of each line before the newline character so lines can be overwritten after created


Structure of texts.txt:
This is a samaple texty from some user'\n'
This is a second sample from some user'\n'

The user can input almost any character they want, excluding new lines;
New lines delimit the end of one texty from the next;
A texty cannot be deleted... ever, unless the account is deactivated


A global map data structure is used to handle data.
The key value is a stringified int, i.e. "1" or "50".
The int represents the user's index in the server.
The data portion of the map contains an instance of my User_Dir class.
The User_Dir class is used mainly to hold a mutex and to hold instances of Lock_File.
Lock_File is another class which has a mutex to lock a specific file based on the member filename (absolute path to a file).
Each User_Dir instance has 4 Lock_File instances corresponding to followees.txt, followers.txt, texts.txt, username.txt
Every user directory stored in the "files" directory has those 4 files specific to that user based on index number.
The user directory name is the user's index number.
Member variables and functions are used to simply getting filepaths and open/closing files.

An All_Users class is also created to handle access to the all_users.txt file which stores every user in the system.
The class handles open/closing the file and also contains a mutex variable to lock the file when necessary.