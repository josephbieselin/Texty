<!-- JOSEPH BIESELIN - Texty Web App -->
<?php
	session_start();

	include 'funcs_and_consts.php';
?>
<!DOCTYPE html>
<html>
<head>
	<title>texty registration submission</title>
</head>
<body>

	<?php
		/*	Create all session variables to be saved
			Session variables used in case passwords are entered wrong, the registry form will be filled out with user info supplied here */
		$fn = $_SESSION['firstname'] = $_POST['firstname'];
		$ln = $_SESSION['lastname'] = $_POST['lastname'];
		$_SESSION['email'] = $_POST['email'];
		$email = strtolower($_SESSION['email']);
		$_SESSION['username'] = $_POST['username'];
		$un = strtolower($_SESSION['username']);
		$pw = $_SESSION['password'] = $_POST['password'];
		$pw_c = $_SESSION['password_check'] = $_POST['password_check'];


		$received_message = talk_to_server($un, $email, $pw, $fn, $ln);
		if($received_message === false) {
			// if false, there was an error getting content from the stream
			echo "<script type=\"text/javascript\">alert('An error occurred. Please try again.'); window.location = \"http://localhost/register.html\";</script>";
		}
		elseif($received_message == ",") {
			// The following executes JavaScript code to alert the user that either the username or email were already taken
			// The user will then be redirected back to the register.html page
			echo "<script type=\"text/javascript\">alert('Username or Email are already used in our system. Please retry registration with a different username/email.'); window.location = \"http://localhost/register.html\";</script>";
		}
		else {
			// User has successfully been registered
			// Displaying data that user entered
			echo "<h1>You're officially a texty-er!</h1>";
			echo "<p><u>Here is the info for your account</u><p>";
			echo "First name: <i>" . $_SESSION['firstname'] . "</i><br/>";
			echo "Last name: <i>" . $_SESSION['lastname'] . "</i><br/>";
			echo "Email: <i>" . $_SESSION['email'] . "</i><br/>";
			echo "Username: <i>" . $_SESSION['username'] . "</i><br/>";
			echo "Password: <b>That's a secret only you know...</b><br/>";
		}

	?>

	<p>Please go back to the <a href="index.php">login page</a> and sign in to start texty-ing</p>

	<!-- Functions for PHP -->
	<?php

		/*
		passes a message to the server with a function to call on the server and user data;
		attempts to received a message back from the server which will indicate success or failure of logging in
		*/
		function talk_to_server($username, $email, $password, $fn, $ln) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = REGISTER_FUNC . "," . $username . "," . $email . "," . $password . "," . $fn . "," . $ln;
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