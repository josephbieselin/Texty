<!-- JOSEPH BIESELIN - Texty Web App -->
<?php 
	include 'funcs_and_consts.php';
?>

<!DOCTYPE html>
<html>
<head>
	<title>Deactivation of texty account</title>
</head>
<body>

	<!-- Remove user from the system with the associated email and password -->
	<?php

		// Check to make sure inputted user data matches our all_users.txt file

		// Set up variables
		$un = strtolower($_POST['username']);
		$email = strtolower($_POST['email']);
		$password = $_POST['password'];

		$received_message = talk_to_server($un, $email, $password);
		if($received_message === false) {
			// if false, there was an error getting content from the stream
			echo "<script type=\"text/javascript\">alert('An error occurred. Please try again.'); window.location = \"http://localhost/deactivate.html\";</script>";
		}
		elseif($received_message == ",") {
			// No match was found for inputted data.
			// Notify user that username, email, password combo was incorrect and they must try again.
			echo "<script type=\"text/javascript\">alert('Your credentials did not match anyone in our system. You will be redirected back to the deactivation form to input your account information again.'); window.location = \"deactivate.html\";</script>";		}
		else {
			// Found matching user data and deleted it
			// Successfully removed all traces of the user assassin style
			echo "You're always welcome back {$un}<br/><br/>";
			echo "All your data has been removed...";
		}
		
	?>

	<!-- Functions for PHP -->
	<?php

		/*
		passes a message to the server with a function to call on the server and user data;
		attempts to received a message back from the server which will indicate success or failure of logging in
		*/
		function talk_to_server($username, $email, $password) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = DEACTIVATE_FUNC . "," . $username . "," . $email . "," . $password . ",,";
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
