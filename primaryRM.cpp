/*
	Texty Web App
	Twitter Clone

	Programmer:
	Joseph Bieselin

	Networking code interfaces PHP on user machine with a server running C++ code.
	"Database" emulated with C++ file handling.
	Threading will be used to handle multiple requests.
	Replication will be implemented as pseudo-servers running in localhost on separate ports.
	
	Each replication manager has their own directory; the name is just a stingified int:
		primaryRM	-->	"1"
		backupRM1	-->	"2"
		backupRM2	--> "3"
		backupRMX	--> "X+1"

	Goals set by different stages for replication part of the project:
		1) Simply handle all data having all data replicated on each "server"
			- i.e. the primary and all replication managers will write all data sent
		2) The primary will offload reads to replication managers
		3) Handle the primary going down
		4) Time for a new leader! A backup now becomes the primary
		5) The primary rises from the dead...?
	
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
*/



#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <stdio.h>		// perror, snprintf
#include <sys/stat.h>	// provide stat functionality for directory checking
#include <string.h>		// provide c string capabilities
#include <strings.h>	// bzero
#include <unistd.h>		// provide functionality for UNIX calls
#include <dirent.h>		// used for filled in directory entries
#include <stdlib.h>		// malloc, calloc, free
#include <typeinfo>
#include <time.h>        // time, ctime
#include <sys/socket.h>  // socket, AF_INET, SOCK_STREAM, bind, listen, accept
#include <netinet/in.h>  // servaddr, INADDR_ANY, htons
#include <mutex>
#include <thread>

using namespace std;



/* ---------------------------------- CONSTANTS ----------------------------------------------*/
// File Handling
#define MAX_PATH 1000		// maximum file path is probably not more than 1000 chars

#define REP_MAN_DIR		"/var/www/html/1"		// directory path for this replication manager
#define FILES_DIR 		"/var/www/html/1/files"	// directory path for files
#define ALL_USERS_TXT	"/var/www/html/1/files/all_users.txt"	// filepath for the master "all_users.txt" file

#define MAX_INDEX_BYTES 10		// maximum number of user indexes that can be used at creation of files: 10 bytes = 999999999 possible indexes for a cstring
#define MAX_USER_INFO_BYTES 118	// maximum number of bytes each line in all_users.txt will be for each user including: user's data, commas, and '\n' character
#define UN_BYTES 17			// maximum number of characters in username based on value limited in PHP file
#define EMAIL_BYTES 41		// maximum number of characters in email based on value limited in PHP file
#define PW_BYTES 17			// maximum number of characters in password based on value limited in PHP file
#define FN_BYTES 21			// maximum number of characters in first name based on value limited in PHP file
#define LN_BYTES 21			// maximum number of characters in last name based on value limited in PHP file
#define GARBAGE_BYTES 70	// length of bytes to hold characters after username and email fields in file handling

#define PHP_MSG_SIZE 130 	// number of characters to hold a message from PHP which could include: 5 chars for function to call, 1 char for a comma, and 118 chars for comma separated user data

// Truncated commands strings passed by PHP to indicate which C++ function to call during network connections
#define REGISTER_STR "regst"	// string for registering a user
#define DEACTIVATE_STR "deact"	// string for deactivating a user
#define LOGIN_STR "login"		// string for logging a user in
#define SEARCH_STR "searc"		// string for searching for user
#define DISPLAY_STR "displ"		// string for displaying a user's page
#define OTHER_DISPLAY_STR "ot_ds"	// string for displaying another user's page
#define TEXTY_STR "texty"		// string for a user to send a texty
#define FOLLOW_STR "follo"		// string for a user to follow another user
#define UNFOLLOW_STR "unfol"	// string for a user to unfollow another user
#define LAST_TEXTY_STR "lastT"	// string to get last texty sent by a user

// Networking
#define	MAXLINE		4096	// max text line length
#define	BUFFSIZE	8192    // buffer size for reads and writes
#define SA struct sockaddr
#define	LISTENQ		1024	// 2nd argument to listen()
#define START_PORT	13001	// smallest port number in use by the RMs
#define END_PORT	13003	// largest port number in use by the RMs

#define THIS_PORT	13001	// this RM's port number
#define HOST_NAME	"localhost"	// the host's name that will relate to an IP address to connect to
/* ---------------------------------- CONSTANTS ----------------------------------------------*/


/* ----------------------------------- CLASSES -----------------------------------------------*/
// Global file class for the master "all_users.txt" file
class All_Users {
public:
	mutable mutex mut;
	fstream fh;
	// filename is absolute path of the file
	string filename;

	// Constructor
	All_Users() : filename(ALL_USERS_TXT) {}

	// Copy Constructor and Assignment Operator deleted
	All_Users(const All_Users& copy) = delete;
	All_Users& operator=(const All_Users& right) = delete;

	// Destructor
	~All_Users()
	{
		if (fh.is_open()) {
			cout << "~All_Users --> closing file" << endl;
			fh.close();
		}

	}

	void open_file()
	{
		cout << "All_Users.open_file()" << endl;
		fh.open(filename);
	}

	void close_file()
	{
		cout << "All_Users.close_file()" << endl;
		fh.close();
	}
};


// Class for locking user files
class Lock_File {
public:
	mutable mutex mut;
	fstream fh;
	// filename is absolute path of the file
	string filename;

	// Constructor
	Lock_File(const string& index_val, const string& name) : filename(FILES_DIR) {filename += "/" + index_val + "/" + name;}
	
	// Copy Constructor and Assignment Operator deleted
	Lock_File(const Lock_File& copy) = delete;
	Lock_File& operator=(const Lock_File& right) = delete;

	// Destructor
	~Lock_File()
	{
		if (fh.is_open()) {
			cout << "~Lock_file --> closing file" << endl;
			fh.close();
		}
	}

	void open_file()
	{
		cout << "Lock_File.open_file() --> " << filename << endl;
		fh.open(filename);
	}

	void close_file()
	{
		cout << "Lock_File.close_file() --> " << filename << endl;
		fh.close();
	}

	// CANNOT CREATE A FILE TO LOCK A FUNCTION BECAUSE UNIQUE_LOCK & LOCK_GUARD RELEASE LOCKS AT THE END OF FUNCTION CALLS
};


// User directory class that can lock a directory to alter files or access Lock_File objects to handle a user's file data
class User_Dir {
public:
	mutable mutex mut;
	string user_index;
	string dir_path;
	// instances of a user's Lock_File objects are created in every directory
	Lock_File followees;
	Lock_File followers;
	Lock_File texts;
	Lock_File username;

	// Constructor
	User_Dir(const string& index_val) : user_index(index_val), dir_path(FILES_DIR), followees(index_val, "followees.txt"), followers(index_val, "followers.txt"), texts(index_val, "texts.txt"), username(index_val, "username.txt") { dir_path += "/" + index_val; }

	// Copy Constructor and Assignment Operator deleted
	User_Dir(const User_Dir& copy) = delete;
	User_Dir& operator=(const User_Dir& right) = delete;

	// Destructor
	~User_Dir() {}

	// unset_index is called when a user is being removed from the system; therefore, indicate that the files no longer exist with a "-1" as the directory index
	void unset_index() { user_index = -1; }

	// open files in a user directory

	void open_followees() 
	{
		if (user_index != "-1")
			followees.open_file();
	}

	void open_followers()
	{
		if (user_index != "-1")
			followers.open_file();
	}

	void open_texts()
	{
		if (user_index != "-1")
			texts.open_file();
	}

	// returns the username of the user corresponding to this indexed directory
	string get_username()
	{
		username.open_file();
		string un;
		getline(username.fh, un);
		username.close_file();
		return un;
	}

	// close files in a user directory
	void close_followees()	{ followees.close_file(); }
	void close_followers()	{ followers.close_file(); }
	void close_texts()		{ texts.close_file(); }
};
/* ----------------------------------- CLASSES -----------------------------------------------*/


/* ------------------------------ GLOBAL VARIABLES -------------------------------------------*/
mutex PHP_ARGS_MUT;		// used when a PHP connection is established and a string needs to be passed to a thread
mutex DIRECTORIES_MUT;	// used when a new user registers or a user deactivates their account (directory is created or removed)

// global All_Users instance to handle locking and unlocking of the master "all_users.txt" file (and opening and closing of it)
All_Users all_user_file;

// global map variable used to handle all user directory and file data
map<string, User_Dir*> USER_DIRECTORIES;

// global vector of ports to other servers; the first index is this server's port number
vector<int> port_nums;
/* ------------------------------ GLOBAL VARIABLES -------------------------------------------*/


/* ---------------------------------------------- FUNCTIONS --------------------------------------------------- */

// Return true if the passed in cstring is a directory; false otherwise
bool is_dir(const char* path)
{
	struct stat buf;
	int status;
	status = stat(path, &buf);
	if (status == -1) {
		// If stat does not return 0, there was an error
		return false;
	}

	if ( (buf.st_mode & S_IFDIR) == 0)
		// Directory was not created -- creating "files" directory now
		return false;

	return true; // no errors and path was determined to be a directory
}

// Return true if the passed in cstring is a file that exists; false otherwise
bool file_exists(const char* name)
{
	struct stat buf;
	int status;
	status = stat(name, &buf);
	return status == 0;
}

// Creates the "files" directory and "all_users.txt" file in that directory if they don't already exist (program exits on any errors in their creation)
void create_users_database()
{
	int status;

	// create a directory where the user info files will go
	if(!is_dir(REP_MAN_DIR.c_str())) {
		// If the RM's index directory doesn't exist, create it with RWX permissions for everyone
		status = mkdir(REP_MAN_DIR.c_str(), S_IRWXU|S_IRWXG|S_IRWXO);
		if (status == -1) {
			cout << endl << "ERROR: mkdir could not create RM's directory" << endl;
			// exit the program if we could not create a directory to store the RM's data
			exit (1);
		}
		chmod(REP_MAN_DIR.c_str(), S_IRWXU|S_IRWXG|S_IRWXO); // give everyone RWX permissions
	}

	// create the files directory inside of the RM's directory
	if (!is_dir(FILES_DIR.c_str())) {
		// If the files directory doesn't exist, create it with RWX permissions for everyone
		status = mkdir(FILES_DIR.c_str(), S_IRWXU|S_IRWXG|S_IRWXO);
		if (status == -1) {
			cout << endl << "ERROR: mkdir could not create files directory" << endl;
			// exit the program if we could not create a directory to store user info
			exit(1);
		}
		chmod(FILES_DIR.c_str(), S_IRWXU|S_IRWXG|S_IRWXO); // give everyone RWX permissions
	}

	// create a file to store all the user's in our system
	if (!file_exists(ALL_USERS_TXT.c_str())) {
		// If the file doesn't exist, create it
		ofstream temp_create_file(ALL_USERS_TXT.c_str());
		temp_create_file.close();

		fstream fh(ALL_USERS_TXT.c_str());

		if (!fh.is_open()) {
			cout << endl << "ERROR: could not open all_users.txt" << endl;
			// exit the program if we could not open the file to store all the users in our system
			exit(2);
		} else { // 1st user created will have an index of 1
			// The first line of the file is the index number that the next registered user will receieve, ie: 1 (since the file did not exist yet)
			string index_line = "1";
			for(size_t i = 1; i < MAX_USER_INFO_BYTES - 1; ++i)
				index_line += ",";
			index_line += "\n";

			fh << index_line; // Each line is 118 chars long, with commands appending user data to set a standard line length, and ends with a newline char
			
			fh.close();
		}
	}
}

// sets up map<string, User_Dir*> USER_DIRECTORIES to contain all files and directories in the system
void create_directory_mappings()
{
	/* Base code taken from StackOverflow URL: http://stackoverflow.com/questions/67273/how-do-you-iterate-through-every-file-directory-recursively-in-standard-c
		Used to read directory entries in the files directory which stores created user directories
		These entries are placed into a map structure which is globally available to use
	*/

	// create a directory pointer and a directory entry pointer
	struct dirent *entry;
	DIR *dp;

	// open the path corresponding the files directory where all the data is stored
	dp = opendir(FILES_DIR.c_str());
	if (dp == NULL) {
		// error opening directory so exit program
		cout << "ERROR: in create_directory_mappings - could not open directory path for 'files'" << endl;
		exit(1);
	}

	// read each directory entry until there are no more entries
	while ((entry = readdir(dp))) {
		// if the directory entry is a directory and not "." or "..", insert it's entry into the USER_DIRECTORIES map
		if ( (entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) ) {
			// insert a new mapping: the key corresponds to a stringified int which is the user's index (and diretory name), and create a User_Dir object passing in the user's stringified int
			USER_DIRECTORIES.insert(pair<string, User_Dir*> (entry->d_name, new User_Dir(entry->d_name)));
		}
	}
	closedir(dp);
}

// sets up the ports_nums vector to contain the int values of all port numbers in use by this application (in our case its a linear set of values starting at 13001)
void create_ports_vector()
{
	for (size_t port_num = START_PORT; port_num <= END_PORT; ++port_num) {
		port_nums.push_back(port_num);
	}
}

// Create 4 .txt files in the passed in directory: followees, followers, texts, username
void create_user_files(const char* dir, const string& un)
{
	char user_path[MAX_PATH + 1];

	// create 4 user files as an ofstream and immediately close it

	strcpy(user_path, dir);
	strcat(user_path, "/");
	strcat(user_path, "followees.txt");
	ofstream f1(user_path); f1.close();

	strcpy(user_path, dir);
	strcat(user_path, "/");
	strcat(user_path, "followers.txt");
	ofstream f2(user_path); f2.close();

	strcpy(user_path, dir);
	strcat(user_path, "/");
	strcat(user_path, "texts.txt");
	ofstream f3(user_path); f3.close();

	// for the username.txt file, input the only line the file will contain, which is the username of this user
	strcpy(user_path, dir);
	strcat(user_path, "/");
	strcat(user_path, "username.txt");
	ofstream f4(user_path);
	f4 << un;
	f4.close();	
}

/*
A new user needs to be created;
Each user has a directory corresponding to their index number;
Each user's directory has 4 .txt files: followees, followers, texts, username
*/
void create_user_dir(const char* user_index, const string& un)
{
	char user_dir[MAX_PATH + 1];
	strcpy(user_dir, FILES_DIR);
	strcat(user_dir, "/");
	strcat(user_dir, user_index);

	int status = mkdir(user_dir, S_IRWXU|S_IRWXG|S_IRWXO);
	if (status == -1) {
		cout << endl << "ERROR: mkdir could not create this new user's directory" << endl;
		return;
	}
	chmod(user_dir, S_IRWXU|S_IRWXG|S_IRWXO); // give everyone RWX permissions

	create_user_files(user_dir, un);

	// lock the directories because a new directory is being added to the system
	lock_guard<mutex> lk(DIRECTORIES_MUT);
	USER_DIRECTORIES.insert(pair<string, User_Dir*>(user_index, new User_Dir(user_index)));	
}

/*
The parameter check will be used in a switch statement and do one of the following 3 checks
	1) If no password is passed, return true if either un or email match the username or email of a user in all_users.txt (used for registering a new user)
	2) If a password is passed, return true if both un and pw match one user's username and password (used for logging in)
	3) Return true if un, email, and pw all match one user's username, email, and password (used for deactivating a user)
*/
bool user_exists(fstream& fh, int check, const string& un, const string& email, const string& pw = "")
{
	// skip the first of the line of the file because it does not contain user info
	string first_line_garbage;
	fh.clear(); fh.seekp(0, ios::beg); getline(fh, first_line_garbage);

	// test info is one line from the all_users.txt file
	// test_info = un,email,index,pw,fn,ln

	// char* test_un = (char*) malloc(UN_BYTES);
	// char* test_email = (char*) malloc(EMAIL_BYTES);
	// char* test_index = (char*) malloc(MAX_INDEX_BYTES);
	// char* test_pw = (char*) malloc(PW_BYTES);

	// char* garbage = (char*) malloc(GARBAGE_BYTES);

	char test_un[UN_BYTES + 1];
	char test_email[EMAIL_BYTES + 1];
	char test_index[MAX_INDEX_BYTES + 1];
	char test_pw[PW_BYTES + 1];

	char garbage[GARBAGE_BYTES + 1];

	while (!fh.eof()) {
		// Test username
		fh.get(test_un, UN_BYTES, ','); 		// comma separated values, so ',' is the delim parameter

		// ',' is the next character in the stream, so just get that
		fh.get();

		fh.get(test_email, EMAIL_BYTES, ','); 	// comma separated values, so ',' is the delim parameter

		// ',' is the next character in the stream, so just get that
		fh.get();

		// Test index
		fh.get(test_index, MAX_INDEX_BYTES, ',');

		// ',' is the next character in the stream, so just get that
		fh.get();

		fh.get(test_pw, PW_BYTES, ',');			// comma separated values, so ',' is the delim parameter

		// garbage and .get() will get the next ',' along with the rest of the user info that is not need for testing 
		fh.get(); //garbage, 2);
		fh.getline(garbage, GARBAGE_BYTES);

		// 1) test username or email, 2) test username and password, 3) test username, email, and password
		switch (check)
		{
			case 1: // Register: both the username and email should be unique
				if( (strcmp(test_un, un.c_str()) == 0) || (strcmp(test_email, email.c_str()) == 0)  ) {
					return true;
				}
			case 2: // Log-in: both the username and password should match
				if( (strcmp(test_un, un.c_str()) == 0) && (strcmp(test_pw, pw.c_str()) == 0) ) {
					return true;
				}
			case 3: // Deactivate: the username, email, and password should match
				if( (strcmp(test_un, un.c_str()) == 0) && (strcmp(test_email, email.c_str()) == 0) && (strcmp(test_pw, pw.c_str()) == 0) ) {
					// lock the user directory to change one its member variable's values
					unique_lock<mutex> lk(DIRECTORIES_MUT);
					unique_lock<mutex> dir_lk(USER_DIRECTORIES[test_index]->mut);

					// for the user that will be soon be removed, set his directory entry index in the global map to "-1" indicating the user and all his/her files are deleted (or in our case in the process of being deleted)
					USER_DIRECTORIES[test_index]->user_index = "-1";

					return true;
				}
		}
	}

	return false; // got to the end of the file and no matches were made so return false
}

// Write over the current line that the fh stream is pointing to with a new index
void write_next_index(fstream& fh, string& next_index)
{
	fh.seekp(0, ios::beg);	// set the file stream to point to the beginning of the file

	for (size_t i = 0; i < (MAX_USER_INFO_BYTES - next_index.size()); ++i)
		next_index += ",";

	//next_index += "\n";

	fh << next_index;
}

// Takes a file handle and adds the strings of user data to the end of the file with the line structure in the comments at the top
void add_user(fstream& fh, const string& un, const string& email, const string& index_val, const string& pw, const string& fn, const string& ln)
{
	string user_line = un + "," + email + "," + index_val + "," + pw + "," + fn + "," + ln + '\n';
	// Clear any flag bits that may impede writing to the file, then set the stream to point to the end of the file
	fh.clear();
	fh.seekp(0, ios::end);
	fh << user_line;
}

/*
The file passed in will be the followees.txt or followers.txt file containing the user we want to remove;
The index corresponds to the index of the user we will be removing
*/
void remove_user_from_files(fstream& fh, ofstream& temp_file, const string& index_val)
{
	string temp, temp_index;

	while (!fh.eof()) {
		// go through each line getting the index for each user
		getline(fh, temp);
		for (size_t i = 0; i < temp.size(); ++i) {
			if (temp[i] == ',')
				break;
			temp_index += temp[i];
		}

		// if the current line's index is not the user we are removing, add them to the new temp file
		if (temp_index != index_val && temp != "") {
			temp_file << temp;
			temp_file << "\n";
		}
		temp_index.clear();
	}
}

/*
Remove the user from all_users.txt;
Remove the user from the followees.txt file of anyone in the user's followers.txt file;
Remove the user from the followers.txt file of anyone in the user's followees.txt file;
Delete the user's followees.txt, followers.txt, and texts.txt files and the user's directory;
Return the passed in username if successful
*/
string remove_user(fstream& fh, ofstream& temp_file, const string& un)
{
	string user_index;
	string temp, temp_username;

	/* ---------------------------- REMOVING USER FROM all_users.txt --------------------------------- */
	// place the first line of all_users.txt into the new temp file
	fh.clear(); fh.seekp(0, ios::beg);
	getline(fh, temp);
	temp_file << temp; temp_file << "\n";

	// add all lines of all_users.txt that don't match the specified username into the new temp file
	while (!fh.eof()) {
		getline(fh, temp);
		size_t i;
		for (i = 0; i < temp.size(); ++i) {
			if (temp[i] == ',')
				break;
			temp_username += temp[i];
		}

		// if the current line's user is the not the user we are removing, add them to the new temp file
		if (temp_username != un && temp != "") {
			temp_file << temp;
			temp_file << "\n";
		} else if (temp_username == un) { // this is the user we are removing
			// first we must parse through the next data value which is the email and i is already at the index of email's first char
			for (++i; i < temp.size(); ++i) {
				if (temp[i] == ',')
					break;
			}
			// get user's index value until the next comma is hit
			for (++i; i < temp.size(); ++i) {
				if (temp[i] == ',')
					break;
				user_index += temp[i];
			}
		}
		temp_username.clear();
	}

	all_user_file.close_file();
	/* ---------------------------- REMOVING USER FROM all_users.txt --------------------------------- */


	/* ------------------- Deleting the user's texts.txt and username.txt files -----------------------*/
	// delete the user's texts.txt file
	unique_lock<mutex> user_text_lk(USER_DIRECTORIES[user_index]->texts.mut);
	remove(USER_DIRECTORIES[user_index]->texts.filename.c_str());
	user_text_lk.unlock();

	// delete the user's username.txt file
	unique_lock<mutex> user_un_lk(USER_DIRECTORIES[user_index]->username.mut);
	remove(USER_DIRECTORIES[user_index]->username.filename.c_str());
	user_un_lk.unlock();
	/* ------------------- Deleting the user's texts.txt and username.txt files -----------------------*/


	// obtain locks on user's followees.txt and followers.txt files
	unique_lock<mutex> user_followee_lk(USER_DIRECTORIES[user_index]->followees.mut);
	unique_lock<mutex> user_follower_lk(USER_DIRECTORIES[user_index]->followers.mut);

	// set up unique_lock pointer object for other user's
	vector<unique_lock<mutex>*> other_lks;
	unique_lock<mutex>* other_user_file_lk;

	string other_user_index;
	char temp_filepath[MAX_PATH + 1];

	/* -------------------- REMOVING USER FROM OTHER USERS' followers.txt file ----------------------- */
	// open followees file
	USER_DIRECTORIES[user_index]->followees.open_file();

	// for each other user in this user's followees.txt file, remove this user from the other user's followers.txt file
	while (!USER_DIRECTORIES[user_index]->followees.fh.eof()) {
		// go through each user in the followees.txt file
		getline(USER_DIRECTORIES[user_index]->followees.fh, temp);
		if (temp != "") {
			// get the other user's index value from the followees.txt file
			for (size_t i = 0; i < temp.size(); ++i) {
				if (temp[i] == ',')
					break;
				other_user_index += temp[i];
			}

			// lock the other user's followers.txt file to remove this user from it
			other_lks.push_back(new unique_lock<mutex> (USER_DIRECTORIES[other_user_index]->followers.mut));

			USER_DIRECTORIES[other_user_index]->followers.open_file();

			// obtain the path to the other user's directory and create temp.txt in it
			strcpy(temp_filepath, USER_DIRECTORIES[other_user_index]->dir_path.c_str());
			strcat(temp_filepath, "/temp.txt");
			ofstream other_followers_temp_file(temp_filepath);

			// remove the current user from the other user's followers.txt file
			remove_user_from_files(USER_DIRECTORIES[other_user_index]->followers.fh, other_followers_temp_file, user_index);
			
			// remove the old file with the user currently in it, and rename the new temp file to its name
			remove(USER_DIRECTORIES[other_user_index]->followers.filename.c_str());
			rename(temp_filepath, USER_DIRECTORIES[other_user_index]->followers.filename.c_str());

			USER_DIRECTORIES[other_user_index]->followers.close_file();
			other_followers_temp_file.close();

			// reset all the variables in this loop for the next iteration
			other_user_index.clear();

			other_user_file_lk = other_lks.back();

			other_user_file_lk->unlock();

			delete other_user_file_lk;

			other_lks.clear();
		}
	}

	// close and delete the user's followee file
	USER_DIRECTORIES[user_index]->followees.close_file();
	remove(USER_DIRECTORIES[user_index]->followees.filename.c_str());
	//user_followee_lk.unlock();
	/* -------------------- REMOVING USER FROM OTHER USERS' followers.txt file ----------------------- */


	/* -------------------- REMOVING USER FROM OTHER USERS' followees.txt file ----------------------- */
	// open followers file
	USER_DIRECTORIES[user_index]->followers.open_file();

	// for each other user in this user's followers.txt file, remove this user from the other user's followees.txt file
	while (!USER_DIRECTORIES[user_index]->followers.fh.eof()) {
		// go through each user in the followers.txt file
		getline(USER_DIRECTORIES[user_index]->followers.fh, temp);
		if (temp != "") {
			// Get the other user's index value from the followers.txt file
			for (size_t j = 0; j < temp.size(); ++j) {
				if( temp[j] == ',')
					break;
				other_user_index += temp[j];
			}

			// lock the other user's followees.txt file to remove this user from it
			other_lks.push_back(new unique_lock<mutex> (USER_DIRECTORIES[other_user_index]->followees.mut));

			USER_DIRECTORIES[other_user_index]->followees.open_file();

			// obtain the path to the other user's directory and create temp.txt in it
			strcpy(temp_filepath, USER_DIRECTORIES[other_user_index]->dir_path.c_str());
			strcat(temp_filepath, "/temp.txt");

			ofstream other_followees_temp_file(temp_filepath);

			// remove the current user from the other user's followees.txt file
			remove_user_from_files(USER_DIRECTORIES[other_user_index]->followees.fh, other_followees_temp_file, user_index);

			// remove the old file with the user currently in it, and rename the new temp file to its name
			remove(USER_DIRECTORIES[other_user_index]->followees.filename.c_str());
			rename(temp_filepath, USER_DIRECTORIES[other_user_index]->followees.filename.c_str());

			USER_DIRECTORIES[other_user_index]->followees.close_file();
			other_followees_temp_file.close();

			// reset all the variables in this loop for the next iteration			
			other_user_index.clear();

			other_user_file_lk = other_lks.back();

			other_user_file_lk->unlock();

			delete other_user_file_lk;

			other_lks.clear();
		}
	}

	// close and delete the user's follower file
	USER_DIRECTORIES[user_index]->followers.close_file();
	remove(USER_DIRECTORIES[user_index]->followers.filename.c_str());
	/* -------------------- REMOVING USER FROM OTHER USERS' followees.txt file ----------------------- */


	/* ----------------------------- deleting the user's directory  ---------------------------------- */
	// For the user being removed, delete his/her directory
	rmdir(USER_DIRECTORIES[user_index]->dir_path.c_str());
	/* ----------------------------- deleting the user's directory  ---------------------------------- */

	return un;
}

/*
register takes in 5 strings: first name, last name, email, username, password;
register stores those strings in a CSV line in all_users.txt in the directory files;
if a failure occurs, a comma is returned;
if successful, the string returned is: un,email,fn,ln
*/
string register_user(const string& un, const string& email, const string& pw, const string& fn, const string& ln)
{
	all_user_file.open_file();

	char index_val[MAX_INDEX_BYTES + 1]; // MAX_INDEX originally set to 1000000 meaning 999999 user indexes could be handled at the creation
	if (!all_user_file.fh.is_open()) {
		cout << endl << "ERROR: could not open all_users.txt" << endl;
		return ",";
	} else { // file is opened	
		all_user_file.fh.getline(index_val, MAX_INDEX_BYTES, ','); // index will get the first line in the file which contains a number followed by the EOL character
		if (user_exists(all_user_file.fh, 1, un, email)) {
			all_user_file.close_file();
			cout << endl << "ERROR: username or email already exist --> " << un << " " << email << " " << pw << " " << fn << " " << ln << endl;
			return ",";
		} else {
			// Add this new user's info to the end of the all_users.txt file
			add_user(all_user_file.fh, un, email, index_val, pw, fn, ln);
			// Create the new user's directory and followees.txt, followers.txt, texts.txt files
			create_user_dir(index_val, un);
		}
	}

	// The index at the beginning of all_users.txt must be incremented and updated in the file
	int next_index;
	next_index = atoi(index_val);
	next_index++;

	// Convert the next_index int value into a string to pass to write_next_index
	char next_index_buffer[MAX_INDEX_BYTES + 1];
	sprintf(next_index_buffer, "%d", next_index);
	string next_index_string(next_index_buffer);
	write_next_index(all_user_file.fh, next_index_string); // write_next_index will move the stream to the beginning of the file
	
	all_user_file.close_file();

	string return_str = un + "," + email + "," + fn + "," + ln;
	return return_str;

	// unique_lock will take care of unlocking the mutex
}

// Takes a username and returns the index of that user (returns "-1" if the user doesn't exist)
string get_user_index(const string& un)
{
	// index is -1 if the username is not associated with any directories
	string the_index = "-1";

	/*
	vector<unique_lock<mutex>*> dir_locks;
	unique_lock<mutex>* lk_ptr;

	for (auto& map_index : USER_DIRECTORIES) {
		dir_locks.push_back(new unique_lock<mutex> (map_index.second->mut));
		if (un == map_index.second->get_username())
			the_index = map_index.second->user_index;
		lk_ptr = dir_locks.back();
		lk_ptr->unlock();
		delete lk_ptr;
		dir_locks.clear();
	}
	*/
	
	unique_lock<mutex>* lk_ptr;	// use a pointer to a lock to loop through multiple mutexes

	for (auto& map_index : USER_DIRECTORIES) {
		// get the lock for one user directory and store the address in lk_ptr
		lk_ptr = new unique_lock<mutex> (map_index.second->mut);

		if (un == map_index.second->get_username()) {
			the_index = map_index.second->user_index;
			break;
		}

		// unlock the lock and clean up space on the heap
		lk_ptr->unlock();
		delete lk_ptr;
	}

	// if we found the user, make sure to release the lock and clean up the heap
	if (the_index != "-1") {
		lk_ptr->unlock();
		delete lk_ptr;
	}

	return the_index;
}

// Takes a message and appends it (along with a newline character) to the user's texts.txt file
bool submit_text(const string& texty, const string& un)
{
	string user_index = get_user_index(un);
	if (user_index == "-1")
		return false;

	// lock the user's texts.txt file because we will be adding a new message to it
	unique_lock<mutex> lk(USER_DIRECTORIES[user_index]->texts.mut);
	USER_DIRECTORIES[user_index]->texts.open_file();

	// write the texty to the end of the texts.txt file
	USER_DIRECTORIES[user_index]->texts.fh.seekp(0, ios::end);	// places the file stream at the end of the file
	USER_DIRECTORIES[user_index]->texts.fh << texty;
	USER_DIRECTORIES[user_index]->texts.fh << "\n";
	USER_DIRECTORIES[user_index]->texts.close_file();

	return true;
}

/*
User (un) wants to follow other user (other_un):
	add the other user's index and username to user's followees.txt file
	add user's index and username to other user's followers.txt file
*/
bool follow_other(const string& un, const string& other_un)
{
	string user_index = get_user_index(un);
	string other_user_index = get_user_index(other_un);

	if ( (user_index == "-1") || (other_user_index == "-1") )
		return false;

	
	/* ---------------- add other user to this user's followees.txt file ------------------- */
	// lock the user's followees.txt file because we will be adding a new user's data to it
	unique_lock<mutex> user_lk(USER_DIRECTORIES[user_index]->followees.mut);
	USER_DIRECTORIES[user_index]->followees.open_file();

	string other_user_data = other_user_index + "," + other_un + "\n";
	// write the other user data to the end of followees.txt
	USER_DIRECTORIES[user_index]->followees.fh.seekp(0, ios::end);	// places the file stream at the end of the file
	USER_DIRECTORIES[user_index]->followees.fh << other_user_data;
	USER_DIRECTORIES[user_index]->followees.close_file();

	// release the lock since the function still has other data to process
	user_lk.unlock();
	/* ---------------- add other user to this user's followees.txt file ------------------- */


	/* ---------------- add user to other user's followers.txt file ------------------- */
	// lock other user's followers.txt file because we will be adding a new user's data to it
	unique_lock<mutex> other_user_lk(USER_DIRECTORIES[other_user_index]->followers.mut);
	USER_DIRECTORIES[other_user_index]->followers.open_file();

	string user_data = user_index + "," + un + "\n";
	// write the user's data to the end of followers.txt
	USER_DIRECTORIES[other_user_index]->followers.fh.seekp(0, ios::end);	// places the file stream at the end of the file
	USER_DIRECTORIES[other_user_index]->followers.fh << user_data;
	USER_DIRECTORIES[other_user_index]->followers.close_file();
	// function about to complete so the locks will be unlocked in unique_lock's destructor
	/* ---------------- add user to other user's followers.txt file ------------------- */

	return true;
}

/*
User (un) wants to unfollow other user (other_un):
	remove other user's index and username from user's followees.txt file
	remove user's index and username from other user's followers.txt file
*/
bool unfollow_other(const string& un, const string& other_un)
{
	string user_index = get_user_index(un);
	string other_user_index = get_user_index(other_un);

	if ( (user_index == "-1") || (other_user_index == "-1") )
		return false;

	char temp_filename[MAX_PATH + 1];

	/* ---------------------------- remove other user from this user's followees.txt file ---------------------------------- */
	// temp_filename will later be renamed to replace the user's followees.txt file
	strcpy(temp_filename, USER_DIRECTORIES[user_index]->dir_path.c_str());
	strcat(temp_filename, "/temp.txt");

	// lock the user directory since we will create temp.txt in it and remove followees.txt in order to update the file content
	unique_lock<mutex> user_lk(USER_DIRECTORIES[user_index]->mut);
	// lock the user's followees.txt file so we transfer its data to temp.txt (excluding the user being deleted)
	unique_lock<mutex> user_file_lk(USER_DIRECTORIES[user_index]->followees.mut);

	USER_DIRECTORIES[user_index]->followees.open_file();
	ofstream temp_file(temp_filename);

	remove_user_from_files(USER_DIRECTORIES[user_index]->followees.fh, temp_file, other_user_index);

	USER_DIRECTORIES[user_index]->followees.close_file();
	temp_file.close();

	// delete followees.txt, and rename temp.txt to followees.txt because it now contains the followees minus the unfollowed one
	remove(USER_DIRECTORIES[user_index]->followees.filename.c_str());
	rename(temp_filename, USER_DIRECTORIES[user_index]->followees.filename.c_str());

	// release the locks in case another thread requires access to the data since this function still has more data to process
	user_lk.unlock();
	user_file_lk.unlock();
	/* ---------------------------- remove other user from this user's followees.txt file ---------------------------------- */


	/* ---------------------------- remove this user from other user's followers.txt file ---------------------------------- */
	// we must now repeat the process, but removing the user from the other user's followers.txt file
	strcpy(temp_filename, USER_DIRECTORIES[other_user_index]->dir_path.c_str());
	strcat(temp_filename, "/temp.txt");

	// obtain locks on the other user's directory and followers.txt file
	unique_lock<mutex> other_user_lk(USER_DIRECTORIES[other_user_index]->mut);
	unique_lock<mutex> other_user_file_lk(USER_DIRECTORIES[other_user_index]->followers.mut);

	USER_DIRECTORIES[other_user_index]->followers.open_file();
	ofstream other_temp_file(temp_filename);

	remove_user_from_files(USER_DIRECTORIES[other_user_index]->followers.fh, other_temp_file, user_index);

	USER_DIRECTORIES[other_user_index]->followers.close_file();
	other_temp_file.close();

	// delete followers.txt, and rename temp.txt to followers.txt because it now contains the followers minus the one who unfollowed
	remove(USER_DIRECTORIES[other_user_index]->followers.filename.c_str());
	rename(temp_filename, USER_DIRECTORIES[other_user_index]->followers.filename.c_str());

	// function about to complete so the locks will be unlocked in unique_lock's destructor
	/* ---------------------------- remove this user from other user's followers.txt file ---------------------------------- */

	return true;
}

/*
Updates a string passed by reference to contain all of the data in a user's followees.txt, followers.txt, and texts.txt files
return_string's contents:
	every followee's index and username pair (comma separated): i1,un1,i2,un2,i3,un3,...,i_N,un_N;
	then there will be a newline character;
	every follower's index and username pair (comma separated): same format as followees
	then there will be a newline character;
	every texty the user has sent, each followed by a newline character (newest texty last);
*/
void display_user_page(const string& un, string& return_str)
{
	string temp_line;
	
	string user_index = get_user_index(un);

	// lock the user's directory because we will be accessing all of the files in the directory
	unique_lock<mutex> user_lk(USER_DIRECTORIES[user_index]->mut);

	// lock the user's followees.txt file while we get its data
	unique_lock<mutex> followee_lk(USER_DIRECTORIES[user_index]->followees.mut);
	USER_DIRECTORIES[user_index]->followees.open_file();

	while (getline(USER_DIRECTORIES[user_index]->followees.fh, temp_line)) {
		return_str += temp_line; return_str += ",";
	}
	USER_DIRECTORIES[user_index]->followees.close_file();
	followee_lk.unlock();

	return_str += "\n"; // end of followees.txt; make newline signify start of followers.txt

	// lock the user's followers.txt file while we get its data
	unique_lock<mutex> follower_lk(USER_DIRECTORIES[user_index]->followers.mut);
	USER_DIRECTORIES[user_index]->followers.open_file();

	while (getline(USER_DIRECTORIES[user_index]->followers.fh, temp_line)) {
		return_str += temp_line; return_str += ",";
	}
	USER_DIRECTORIES[user_index]->followers.close_file();
	follower_lk.unlock();

	return_str += "\n"; // end of followers.txt; make newline signify start of texts.txt

	// get data in texts.txt; put strings a vector because newest texty will be the last line of the file, but it should be the first texty in return_str
	vector<string> textys;

	// lock the user's texts.txt file while we get its data
	unique_lock<mutex> texts_lk(USER_DIRECTORIES[user_index]->texts.mut);
	USER_DIRECTORIES[user_index]->texts.open_file();

	while (getline(USER_DIRECTORIES[user_index]->texts.fh, temp_line)) {
		textys.push_back(temp_line);
	}
	USER_DIRECTORIES[user_index]->texts.close_file();
	texts_lk.unlock();

	// start from the end of the vector, thus adding the newest texty to return_str first, and the oldest texty last
	for (size_t i = textys.size(); i > 0; --i) {
		return_str += textys[i-1];
		return_str += "\n"; // each texty should be separated by a newline
	}
	
	return; // no need to return anything because the string with updated data was passed by reference
}

// will return the most recent texty if the user's texts.txt file is opened; '\n' will be returned if it fails or there's no textys
string get_last_texty(const string& un)
{
	string temp_line;

	string newest_texty = "";
	string user_index = get_user_index(un);
	
	// -1 indicates the user directory does not exist and therefore the user does not exist
	if (user_index == "-1")
		return "\n";

	// lock the user's texts.txt file so no writes are performed while we retrieve the data
	unique_lock<mutex> lk(USER_DIRECTORIES[user_index]->texts.mut);
	USER_DIRECTORIES[user_index]->texts.open_file();

	if (!USER_DIRECTORIES[user_index]->texts.fh.is_open()) {
		return "\n";
	}

	while (getline(USER_DIRECTORIES[user_index]->texts.fh, temp_line)) {
		if (temp_line != "")
			newest_texty = temp_line; // if temp_line holds data, newest_texty will get that data
	}
	// just got to the end of the file because there were no more lines to get
	USER_DIRECTORIES[user_index]->texts.close_file();

	if (newest_texty == "")
		return "\n";

	return newest_texty; 
}

/*
Search for the username or email of another user:
Return formatted user data in a string if the username/email is found: string = "username,email,index,firstname,lastname"
Return a comma, ',' if the username/email is not found
*/
string search(const string& search_str)
{
	all_user_file.open_file();

	// skip the first of the line of the file because it does not contain user info
	string first_line_garbage;
	getline(all_user_file.fh, first_line_garbage);

	string test_un, test_email, user_data, return_user_data;

	while (!all_user_file.fh.eof()) {
		// get the next line of one user's data
		getline(all_user_file.fh, user_data);
		if (user_data != "") {
			size_t i;
			// get the username to test
			for (i = 0; i < user_data.size(); ++i) {
				if (user_data[i] == ',')
					break;
				test_un += user_data[i];
			}
			// get the email to test
			for (++i; i < user_data.size(); ++i) {
				if (user_data[i] == ',')
					break;
				test_email += user_data[i];
			}

			// if the username or email match, get the first and last name to display also
			if ( (test_un == search_str) || (test_email == search_str) ) {
				all_user_file.close_file(); // close the file since we have all the data needed
				return_user_data = test_un + "," + test_email + ","; // add username and email to string that will be returned
				// get the index and store 
				for (++i; i < user_data.size(); ++i) {
					if (user_data[i] == ',') {
						return_user_data += ",";
						break;
					}
					return_user_data += user_data[i];
				}
				// get the password but don't store it
				for (++i; i < user_data.size(); ++i) {
					if (user_data[i] == ',')
						break;
				}
				// put the rest of the line in return_user_data which already has the username, email, and index stored with comma separation
				for (++i; i < user_data.size(); ++i) {
					return_user_data += user_data[i];
				}

				return return_user_data; // return the formatted user data: username,email,firstname,lastname
			}
		}
		// the username or password was not found so clear the username and email just tested
		test_un.clear(); test_email.clear();
	}

	all_user_file.close_file();

	return ","; // return a commas since no username/email matched
}

/* ---------------------------------------------- FUNCTIONS --------------------------------------------------- */



/* ------------------------ handle the PHP message passed over the connection via an open socket ---------------------- */

/*
Takes a string where the first 5 chars will represent a function that PHP has requested (examples below):
	regst = register user;
	deact = deactivate a user's account;
	login = login a user;
	searc = search for a user;
	displ = display the logged in user's home page;
	ot_ds = display other user's page;
Data to be used with this function call will be a string which is passed by referenced as return_str:
	contains comma - "," - if requested action is denied/failed;
	contains requested data if all goes well;
*/
void handle_php_args(const string php_args, int connfd)
{
	// php_args will contain a function to call and necessary user data; data is comma delimited
	string func_to_call = "";
	string un = "";
	string email = "";
	string pw = "";
	string fn = "";
	string ln = "";
		
	string return_str;

	size_t i = 0;

	/*
	This function will also handle sending messages over sockets to other Replication Managers if necessary.

	If the last char in php_args is 'c', this implies another RM has made a connection with us.
	The connection was made because the php_args string passed had contents that called a function which altered data.
	This alteration of data could happen if a new user registers, a user deactivates an account, a user submits a new text, etc...
	Because data has been updated in the files, all RMs need to be made aware of this.
	To make all the RMs aware of the change, we must connect to them via sockets.
	On these sockets, we will pass php_args with the last char being set to 'c' indicating replication must happen.

	Replication calls only happen when a change in data happens; reads do not require replicated calls.
	For example, a user attempting to login only requires the reading of a file which the RM connected to should handle by itself.
	*/

	// if the last char in php_args is not 'c', then we will need to connect to other RMs if data is updated
	bool replicate_data = (php_args[php_args.length() - 1] != "c");

	// since there was no 'c' ending char in php_args, we must send data back over the socket to the PHP connection that requested information
	bool send_data = replicate_data;

	// get all possible values from php_args in order: function,username,email,password,firstname,lastname

	for (i; i < php_args.size(); ++i) {
		if (php_args[i] == ',')
			break;
		func_to_call += php_args[i];
	}
	for (++i; i < php_args.size(); ++i) {
		if (php_args[i] == ',')
			break;
		un += php_args[i];
	}
	for (++i; i < php_args.size(); ++i) {
		if (php_args[i] == ',')
			break;
		email += php_args[i];
	}
	for (++i; i < php_args.size(); ++i) {
		if (php_args[i] == ',')
			break;
		pw += php_args[i];
	}
	for (++i; i < php_args.size(); ++i) {
		if (php_args[i] == ',')
			break;
		fn += php_args[i];
	}
	for (++i; i < php_args.size(); ++i) {
		if (php_args[i] == ',')
			break;
		ln += php_args[i];
	}

	// set up a possible lock call on all_users.txt
	unique_lock<mutex> lk(all_user_file.mut, defer_lock);

	// functions that do not complete their desired task for whatever reason (bad input, failed calls, etc) return a comma - ','
	
	if (func_to_call == REGISTER_STR) {
		lk.lock();

		return_str = register_user(un, email, pw, fn, ln); // returns username,email,firstname,lastname if successful
		
		if (return_str == ",")
			replicate_data = false; // if a ',' was returned, registration wasn't successful so do not replicate
	}

	else if (func_to_call == DEACTIVATE_STR) {
		lk.lock();
		all_user_file.open_file();

		if (!user_exists(all_user_file.fh, 3, un, email, pw)) {
			all_user_file.close_file();
			return_str = ",";

			replicate_data = false; // if the user does not exist, do not replicate trying to remove the user in other RMs
		} else {
			// the user exists, so remove them from all_users.txt by creating a temp file without them, then renaming that temp file to all_users.txt
			char temp_filename[MAX_PATH + 1];
			strcpy(temp_filename, FILES_DIR);
			strcat(temp_filename, "/temp.txt");
			ofstream temp_file(temp_filename);

			string remove_str = remove_user(all_user_file.fh, temp_file, un); // returns username if successful

			// remove the old all_users.txt and rename the temp file with updated user info to all_users.txt
			all_user_file.close_file(); temp_file.close();
			remove(all_user_file.filename.c_str());
			rename(temp_filename, all_user_file.filename.c_str());

			return_str = remove_str;
		}
	}

	else if (func_to_call == LOGIN_STR) {
		lk.lock();
		all_user_file.open_file();

		if (!user_exists(all_user_file.fh, 2, un, email, pw)) {
			all_user_file.close_file();
			return_str = ",";
		} else {
			all_user_file.close_file();
			return_str = un; // returns username
		}

		replicate_data = false; // login is a read operation so we do not need to replicate a read in other RMs
	}

	else if (func_to_call == SEARCH_STR) {
		lk.lock();

		string search_string; // will contain a username or email to search for
		search_string = ( (un != "") ? un : email);
		return_str = search(search_string); // returns username,email,firstname,lastname if user is found

		replicate_data = false; // search is a read operation so we do not need to replicate a read in other RMs
	}

	else if (func_to_call == DISPLAY_STR) {
		string return_string; // will contain all the data from the user's followees, followers, and texts .txt files
		display_user_page(un, return_string); // return_string passed by referenced and changed in the function
		return_str = return_string;
		/*
		return_string's contents:
			every followee's index and username pair (comma separated): i1,un1,i2,un2,i3,un3,...,i_N,un_N;
			then there will be a newline character;
			every follower's index and username pair (comma separated): same format as followees
			then there will be a newline character;
			every texty the user has sent, each followed by a newline character (newest texty first);
		*/

		replicate_data = false; // display page is a read operation so we do not need to replicate a read in other RMs
	}

	else if (func_to_call == TEXTY_STR) {
		string the_message = email; // texty gets sent as 3rd value which happens to be the email field
		if (submit_text(the_message, un)) {
			return_str = un;
		} else {
			return_str = ",";

			replicate_data = false; // if submitting a text was unsuccessful, do not replicate
		}
	}

	else if (func_to_call == LAST_TEXTY_STR) {
		lk.lock();

		return_str = get_last_texty(un);

		replicate_data = false; // getting the last text is a read operation so we do not need to replicate a read in other RMs
	}

	else if (func_to_call == FOLLOW_STR) {
		string other_un = email;	// the other user's username that we are trying to follow is passed as the email field
		if (follow_other(un, other_un)) {
			return_str = un;
		} else {
			return_str = ",";

			replicate_data = false; // if following another user was unsuccessful, do not replicate
		}
	}

	else if (func_to_call == UNFOLLOW_STR) {
		string other_un = email;	// the other user's username that we are trying to unfollow is passed as the email field
		if (unfollow_other(un, other_un)) {
			return_str = un;
		} else {
			return_str = ",";

			replicate_data = false; // if unfollowing another user was unsuccessful, do not replicate
		}
	}

	else { // if the string does not match somehow, there was an error so return a comma
		cout << "Error in php_args: function to call was a bad value" << endl;
		return_str = ",";

		replicate_data = false; // something went wrong so DEFINITELY do not replicate
	}

	// if we must send our return_str back over a socket, write to the connection file descriptor
	if (send_data) {
		// write the message over the network back to the PHP that opened a socket
		unsigned int return_message_size = return_str.size();
		if ( return_message_size != write(connfd, return_str.c_str(), return_str.size()) ) {
			perror("write to connection failed for primary\n");
		}
	}

	// if we want to replicate data, we must connect to all other RMs that are not us and send the php_args string
	if (replicate_data) {
		/*
		For each other RM, create a connection and send the php_args with 'c' as the last char to indicate its a replication call.
		The connected RM will not send any data back because it only needs to update its own data, so connections will be closed immediately after data is sent.
		*/

		php_args[php_args.length() - 1] = "c"; // set the last char to 'c' so the RM knows it just needs to update its own data

		int listenfd, sockfd;
		struct sockaddr_in rm_addr;
		struct hostent *host_entry;
		int addrlen = sizeof(SA);

		// Try and create a socket until it is successful
		while ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			// print an error message for every erronous attempt
			fprintf(stderr, "Unable to create socket for data replication.\n");
		}

		// for each port (excluding this RM's port), attempt to send the php_args to each RM
		for (size_t i = START_PORT; i <= END_PORT; ++i) {
			// no need to talk to ourselves, that'd be kind of weird
			if (i != THIS_PORT) {
				// zero out the replication manager's socket address
				memset(&rm_addr, 0, sizeof(rm_addr));
				rm_addr.sin_family = AF_INET; // address family: AF_INET (from linux man pages)

				// get info about the host we're trying to connect to
				host_entry = gethostbyname(HOST_NAME); // host_entry->h_addr contains any old IP address from the host

				if (!host_entry) {
					fprint(stderr, "Unable to get host info for %s\n", HOST_NAME);
					// couldn't get the host info so go to the next port
					continue;
				}
				// load the socket's s_addr with the IP of host
				if (inet_aton(host_entry->h_addr, &rm_addr.sin_addr.s_addr) == 0) {
					fprintf(stderr, "Unable to load the IP address into s_addr.\n");
				}
				rm_addr.sin_port = htons(i); // one of the other RMs ports
				if ((sockfd = connect(listenfd, (SA *) rm_addr, sizeof(rm_addr))) == -1) {
					// a connection could not be made which means the RM is probably down
					fprintf(stderr, "Unable to connect to the Replication Manager on Port %d.\n", i);
					// since the connection failed, continue onto the next RM port number
					continue;
				}

				// now data can be written over sockfd to the other RM
				if ( PHP_MSG_SIZE != write(sockfd, php_args.c_str(), PHP_MSG_SIZE) ) {
					fprintf(stderr, "Write to connection failed for Port %d.\n", i);
				}

				// close the socket connection to the other RM so the next RM can be connected to
				close(sockfd);
			}
		}

		// done sending replicate data commands to other RMs so close this RM's socket
		close(listenfd);
	}

	close(connfd);
}

/* ------------------------ handle the PHP message passed over the connection via an open socket ---------------------- */



/*
  From Stevens Unix Network Programming, vol 1.
  Minor modifications by John Sterling
*/

int main(int argc, char **argv) {

    int			listenfd, connfd;  // Unix file descriptors
    struct sockaddr_in	servaddr;          // Note C use of struct
    //char		buff[MAXLINE];
    //time_t		ticks;

    // 1. Create the socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Unable to create a socket for primary");
        exit(1);
    }

    // 2. Set up the sockaddr_in

    // zero it.  
    // bzero(&servaddr, sizeof(servaddr)); // Note bzero is "deprecated".  Sigh.
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET; // Specify the family
    // use any network card present
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(THIS_PORT);	// daytime server

    // 3. "Bind" that address object to our listening file descriptor
    if (bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) == -1) {
        perror("Unable to bind port for primary");
        exit(2);
    }

    // 4. Tell the system that we are going to use this sockect for
    //    listening and request a queue length
    if (listen(listenfd, LISTENQ) == -1) {
        perror("Unable to listen for primary");
        exit(3);
    }
    
    // initialize directories, files and mapping data for the simulated database
    create_users_database();		// creates the files directory and all_users.txt if they don't already exist
    create_directory_mappings();	// updates the global map variable

    vector<thread> threads;		// multiple threads will be created and this vector will store each one
    unique_lock<mutex> lk(PHP_ARGS_MUT, defer_lock);	// locked will be used so the proper PHP message will be passed to each thread

    for ( ; ; ) {
        // 5. Block until someone connects.
        //    We could provide a sockaddr if we wanted to know details of whom
        //    we are talking to.
        //    Last arg is where to put the size of the sockaddr if
        //    we asked for one
		fprintf(stderr, "Ready to connect for primary.\n");
	        if ((connfd = accept(listenfd, (SA *) NULL, NULL)) == -1) {
	            perror("accept failed for primary");
	            exit(4);
		}
		fprintf(stderr, "Connected for primary\n");

        // We now have a connection.  Do whatever our task is.

		/* ---------------------------- HANDLING MESSAGE PASSING OVER NETWORK -------------------------------------*/
		char php_args[PHP_MSG_SIZE + 1];	// first 5 bytes will be the function to call in C++; remaining message will be passed in user info
		int read_bytes = read(connfd, php_args, PHP_MSG_SIZE); // number of bytes read from connection file descriptor
		if (read_bytes == PHP_MSG_SIZE) {
			// handle_php_args will perform necessary C++ functions based on PHP's message passed and write to the connection fd
			lk.lock();
			threads.push_back( thread( [php_args, connfd] { handle_php_args(php_args, connfd); } ) );
			lk.unlock();
			// handle_php_args also closes the connfd once completed

		} else { // the connection did not read as many bytes as was passed
			perror("read from connection failed for primary");
		}
    }

    // join the threads so they run to completion
    for (auto& t : threads)
    	t.join();
}








/* Some tester code I was using

    vector<thread> threads;

    char php_args[PHP_MSG_SIZE + 1];
	string m1, m2, m3, m4, m5, m6, m7;
	// append a new thread to the threads vector and join it so that it finishes execution

	unique_lock<mutex> lk(PHP_ARGS_MUT, defer_lock);


	lk.lock();
	strcpy(php_args, "searc,lexA,ml@gmail.com,password1,mitch,lee,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//string args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m1] { handle_php_args(php_args, my_cwd, files_cwd, m1); } ) );
	lk.unlock();

	lk.lock();
	strcpy(php_args, "follo,lork,lexA,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m2] { handle_php_args(php_args, my_cwd, files_cwd, m2); } ) );
	lk.unlock();

	lk.lock();
	strcpy(php_args, "follo,lork,joebee,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m3] { handle_php_args(php_args, my_cwd, files_cwd, m3); } ) );
	lk.unlock();

	lk.lock();
	strcpy(php_args, "deact,doge,dog@verizon.net,password1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m4] { handle_php_args(php_args, my_cwd, files_cwd, m4); } ) );
	lk.unlock();

	lk.lock();
	strcpy(php_args, "texty,lexA,my first texty in 4e,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m5] { handle_php_args(php_args, my_cwd, files_cwd, m5); } ) );
	lk.unlock();

	lk.lock();
	strcpy(php_args, "follo,lexA,joebee,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m6] { handle_php_args(php_args, my_cwd, files_cwd, m6); } ) );
	lk.unlock();

	lk.lock();
	strcpy(php_args, "displ,joebee,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
	//args = php_args;
	threads.push_back( thread( [php_args, my_cwd, files_cwd, &m7] { handle_php_args(php_args, my_cwd, files_cwd, m7); } ) );
	lk.unlock();



	//handle_php_args(php_args, my_cwd, files_cwd, message_to_return);
	for (auto& t : threads) t.join();

	cout << "message 1: " << m1 << endl; // 
	cout << "message 2: " << m2 << endl; // 
	cout << "message 3: " << m3 << endl; // 
	cout << "message 4: " << m4 << endl; // 
	cout << "message 5: " << m5 << endl; // 
	cout << "message 6: " << m6 << endl; // 
	cout << "message 7: " << m7 << endl << endl; // 

	for (auto& i : USER_DIRECTORIES) {
		cout << "key: " << i.first << " - index: " << i.second->user_index << " - dir_path: " << i.second->dir_path << ", files: " << i.second->followees.filename << " " << i.second->followers.filename << " " << i.second->texts.filename << endl << endl;
	}


	//chdir(my_cwd);
	// get the return message's size to confirm the correct number of bytes are written to the file stream
	//unsigned int return_message_size = message_to_return.size();




}
*/