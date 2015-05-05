<!-- JOSEPH BIESELIN - Texty Web App -->
<?php
	session_start();

	// Setting GLOBAL CONSTANTS for use with Networking code
	define("REGISTER_FUNC", "regst");		// function name passed to C++ to register a user
	define("DEACTIVATE_FUNC", "deact");		// function name passed to C++ to deactivate a user
	define("LOGIN_FUNC", "login");			// function name passed to C++ for logging a user in
	define("SEARCH_FUNC", "searc");			// function name passed to C++ to search for a user
	define("DISPLAY_PAGE_FUNC", "displ");	// function name passed to C++ to display a user's page
	define("TEXTY_FUNC", "texty");			// function name passed to C++ to submit a texty
	define("FOLLOW_FUNC", "follo");			// function name passed to C++ to follow another user
	define("UNFOLLOW_FUNC", "unfol");		// function name passed to C++ to unfollow another user
	define("LAST_TEXTY_FUNC", "lastT");		// function name passed to C++ to get a user's most recent texty
	//define("PORT", "13001");				// port number directed to the primary Replication Manager server socket (excluding defaults like 80)
	define("PHP_MSG_SIZE", 130);			// number of chars PHP will send over a socket
	define("BUFFSIZE", 8192);				// buffer for reading and writing over network socket
	define("CPP_FILE", "texty.cpp");		// used for executing the C++ code in PHP

	// range of port numbers used to connect to Replication Manager "servers" running in C++ code
	define("MIN_PORT", 13001);
	define("MAX_PORT", 13003);

	// returns a random number from MIN_PORT to MAX_PORT
	function get_random_port() {
		$port_num = rand(MIN_PORT, MAX_PORT);
		return "{$port_num}";
	}

	// appends commas to the passed in string so the returned string is of length PHP_MSG_SIZE 
	function append_commas($the_string) {
		$string_len = strlen($the_string);
		if($string_len < PHP_MSG_SIZE) {
			// the_string that will be sent over the socket must have a length of PHP_MSG_SIZE
			// append commas until the string length is PHP_MSG_SIZE
			for($i = $string_len; $i <= PHP_MSG_SIZE; $i++) {
				$the_string .= ",";
			}
		}
		return $the_string;
	}
?>