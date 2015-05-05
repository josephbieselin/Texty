<!-- JOSEPH BIESELIN - Texty Web App -->
<?php 
	session_start();

	// Set up variables
	$un = strtolower($_POST['username']);
	$_SESSION['username'] = $un;
	$password = $_POST['password'];

	include 'funcs_and_consts.php';

?>

<!DOCTYPE html>
<html>
<head>
	<title>User Verification</title>
</head>
<body>

<?php
	

	$received_message = talk_to_server($un, $password);
	if($received_message === false) {
		// if false, there was an error getting content from the stream
		echo "<script type=\"text/javascript\">alert('An error occurred. Please try signing in again.'); window.location = \"http://localhost/index.php\";</script>";
	}
	elseif($received_message == ",") {
		// No match was found for inputted data.
		// Notify user that username or password was incorrect and they must try to log in again.
		echo "<script type=\"text/javascript\">alert('Username or Password were incorrect. Please try signing in again.'); window.location = \"http://localhost/index.php\";</script>";
	}
	else {
		// Found a match so we direct the user to their homepage
		echo "<script type=\"text/javascript\">window.location = \"http://localhost/user_page.php\";</script>";
	}


	/*
	passes a message to the server with a function to call on the server and user data;
	attempts to received a message back from the server which will indicate success or failure of logging in
	*/
	function talk_to_server($username, $password) {
		//exec(CPP_FILE $arr); // run the C++ program to start a connection; all output goes into $arr
		$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
		if(!$socket_fd) {
			echo 'localhost port error' . "<br/>";
			echo "$errstr ($errno)";
			exit(1);
		}
		// $message is the string that will be sent over the open socket to the server
		// It will be in the format: function_name,username,email,password,firstname,lastname
		// String will delimit different data fields by commas, even if a data field is empty
		$message = LOGIN_FUNC . "," . $username . ",," . $password . ",,";
		$message = append_commas($message); // append commas until message's length is PHP_MSG_SIZE
		fwrite($socket_fd, $message);
		$received_message;
		stream_set_blocking($socket_fd, 1); // Blocks the stream until data is available on it
		$received_message = stream_get_contents($socket_fd);
		fclose($socket_fd);
		return $received_message;
	}


?>

</body>
</html>