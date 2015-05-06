<!-- JOSEPH BIESELIN - Texty Web App -->
<?php
	session_start();

	include 'funcs_and_consts.php';
?>

<!DOCTYPE html>
<html>
<head>
	<title>search texty</title>
</head>
<body>

	<h3>
		<!-- Link back to user's homepage -->
		<p><a href="user_page.php">Home</a></p>
	</h3>

	<?php
		// Set variables to the values in the global SESSION variable if possible
		if(isset($_SESSION['username'])) {
			$username = strtolower($_SESSION['username']);
		}
		// If we don't know who is currently logged in, die.
		else { die("Please log back in."); }

		if(isset($_GET['search_term'])) {
			$search_term = strtolower($_GET['search_term']);
		}
		// If we don't know what to search, die.
		else { die("Please try search again."); }


		$received_message = talk_to_server($search_term);
		if($received_message === false) {
			// if false, there was an error getting content from the stream
			echo "<script type=\"text/javascript\">alert('An error occurred. Please try again.'); window.location = \"http://localhost/user_page.php\";</script>";
		}
		elseif($received_message == ",") {
			// The user will then be redirected back to their home page
			echo "<script type=\"text/javascript\">alert('Username / Email not found. Taking you back to your home page where you may search again.'); window.location = \"http://localhost/user_page.php\";</script>";
		}
		else {
			// A match was found in the username or email so create a link to that user's page
			// link_to_user_page takes in the user's data except for the password
			// $received_message contains the search for user's: username,email,firstname,lastname in that format
			$other_user_array = explode(",", $received_message);
			link_to_user_page($other_user_array[0], $other_user_array[1], $other_user_array[2], $other_user_array[4], $other_user_array[5]);
		}

	?>

	<!-- Functions for PHP -->
	<?php
		// link_to_user_page takes in a user's data and displays a link to their page
		function link_to_user_page($link_username, $link_email, $link_index, $link_firstname, $link_lastname) {
			echo "<a href='other_user.php?index=$link_index&username=$link_username'>$link_username</a> | Email: <b>$link_email</b> | Name: <b>$link_firstname $link_lastname</b><br/>";
		}

		/*
		passes a message to the server with a function to call on the server and user data;
		attempts to received a message back from the server which will indicate success or failure of logging in
		*/
		function talk_to_server($search_str) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = SEARCH_FUNC . "," . $search_str;
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
