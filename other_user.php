<!-- JOSEPH BIESELIN - Texty Web App -->
<?php
	session_start();

	include 'funcs_and_consts.php';

	// Set variables to the values in the global SESSION variable if possible
	if(isset($_SESSION['username'])) {
		$username = strtolower($_SESSION['username']);
	}
	// If we don't know who is currently logged in, die.
	else { die("Please log back in."); }

	// GET username passed in by the URL
	if(isset($_GET['username'])) {
		$other_username = strtolower($_GET['username']);
	}
	// If we can't get the other user's name, die.
	else { die("Couldn't retrieve desired user's data."); }

	// If user and other_user's username values are the same, go to the user's homepage
	if($username == $other_username) {
		header("Location: user_page.php");
	}

	// Check to see if a follow/unfollow request was submitted
	if(isset($_POST['fol_unfol'])) {
		// If there was a request to follow/unfollow, submit that request
		submit_fol_unfol($username, $other_username, $_POST['fol_unfol']);
	}

	$received_message = talk_to_server_display($other_username);
	if($received_message === false) {
		// if false, there was an error getting content from the stream
		echo "<script type=\"text/javascript\">alert('An error occurred. Please sign in again.'); window.location = \"http://localhost/index.php\";</script>";
	}
	elseif($received_message == ",") {
		// Something went wrong so direct user back to login
		echo "<script type=\"text/javascript\">alert('Apologies, something went wrong. Please sign in again.'); window.location = \"http://localhost/index.php\";</script>";
	}
	else {
		/*
		return_string's contents:
			every followee's index and username pair (comma separated): i1,un1,i2,un2,i3,un3,...,i_N,un_N;
			then there will be a newline character;
			every follower's index and username pair (comma separated): same format as followees
			then there will be a newline character;
			every texty the user has sent, each followed by a newline character (newest texty first);
		*/

		/*
		user_data[0] = followees info:
			i1,un1,i2,un2,i3,un3,....,i_N,un_N
		user_data[1] = followers info:
			i1,un1,i2,un2,i3,un3,....,i_N,un_N
		user_data[2] = most recent texty (if any at all)
		user_data[3] = 2nd most recent texty (if there is one)
		...
		...
		user_data[N] = the Nth - 2 texty (also the first texty the user shared)
		-------------------------------------------------------------------------------------------------------------
		The data above will be converted into arrays where each array index is equivalent to a line in the user files;
		Each index of followees_file_array will contain the string i_X,un_X;
		Each index of followers_file_array will contain the string i_Y,un_Y;
		Each index of texts_file_array will be one texty;
		*/
		//echo "$user_data<br/>";
		$user_data = explode("\n", $received_message);
		array_pop($temp_followers_array);
		// set up followees_file_array
		$temp_followees_array = explode(",", $user_data[0]);
		$followees_file_array;
		$str1; $str2; $str_index;
		$count = count($temp_followees_array);
		for($i = 0; $i < $count; $i++) {
			$str1 = $temp_followees_array[$i]; // index of the user for this line
			$i++;
			$str2 = $temp_followees_array[$i]; // username of the user for this line
			$str_index = $str1 . "," . $str2;
			$followees_file_array[] = $str_index; // add str_index to the end of the array
		}
		// set up followers_file_array
		$temp_followers_array = explode(",", $user_data[1]);
		array_pop($temp_followers_array); // there was an extra comma with no value after it
		$followers_file_array;
		$count = count($temp_followers_array);
		for($i = 0; $i < $count; $i++) {
			$str1 = $temp_followers_array[$i]; // index of the user for this line
			$i++;
			$str2 = $temp_followers_array[$i]; // username of the user for this line
			$str_index = $str1 . "," . $str2;
			$followers_file_array[] = $str_index; // add str_index to the end of the array		
		}

		// unset the first two elements (followees & followers) so all that's left is textys
		array_shift($user_data); array_shift($user_data);
		$texts_file_array = $user_data;
	}

?>

<!DOCTYPE html>
<html>
<head>
	<title>texty</title>
</head>
<body>

	<h3>
		<!-- Link back to user's homepage -->
		<a href="user_page.php">Home</a>
	</h3>
	<p>
		<!-- Search Form -->
		<form action="search.php" style="display:inline; float:left">
			<input type="text" name="search_term" placeholder="Search for users">
			<input type="submit" value="Search"> <!-- style="visibility: hidden" -->
		</form>
		<a href="index.php" style="float: right">Logout</a>
	</p>
	<div style="clear: both"></div>

	<hr/>

	<!--
	Determine if the other user we are looking at is someone that the logged in user follows.
	If we follow them, provide an "Unfollow" button.
	If we don't follow them, provide a "Follow" button. -->
 	<?php

 		// determine if we are a follower of this user
 		$cond = False;
 		$count = count($followers_file_array);
 		$followers_data;
 		for($i = 0; $i < $count; $i++) {
 			$followers_data = explode(",", $followers_file_array[$i]);
 			if($followers_data[1] == $username) {
 				$cond = True;
 				break;
 			}
 		}


		echo "<h2><p style='font-family:Lucida Console, Monaco, monospace; display: inline'>{$other_username}'s Stuff";
		// Unfollow buttom
		if($cond) {
			echo "<form method='post' action='other_user.php?&username=$other_username' style='display: inline'>";
			echo "<input type='text' value='unfollow' name='fol_unfol' style='visibility: hidden'>";
			echo "<input type='submit' value='Unfollow'></form>";
		}
		// Follow button
		else {
			echo "<form method='post' action='other_user.php?&username=$other_username' style='display: inline'>";
			echo "<input type='text' value='follow' name='fol_unfol' style='visibility: hidden'>";
			echo "<input type='submit' value='Follow' style='display: inline'></form>";
		}
		echo "</p></h2>";

	?>



	<!-- User data: Followees, Texty-s, Followers -->
	<table width="100%">
		<tr>
			<!-- padding: top right bottom left; -->
			<th align="left" width="28%" style="padding: 1px 2px 3px 1px">Following</th>
			<th align="center" width="44%" style="padding: 3px 2px 3px 2px">Past Messages</th>
			<th align="right" width="28%" style="padding: 1px 1px 3px 2px">Followers</th>
		</tr>
	
		<!-- Sample Table Data Contents -->
		<!-- 			<tr>
						<td align="left" width="28%" style="padding: 1px 2px 3px 1px"><a href="other_user.php">FOLLOWED PERSON</a><br/><textarea>texty</textarea></td>
						<td align="center" width="44%" style="padding: 3px 2px 3px 2px">
							<?php
							//	echo "<textarea rows=\"2\" cols=\"60\" disabled=\"disabled\">texty</textarea>";
							?>
						</td>
						<td align="right" width="28%" style="padding: 1px 1px 3px 2px"><a href="other_user.php">FOLLOWER</a></td>
					</tr>
		 -->


		<?php
			

			$i = 0;
			$followees_size = count($followees_file_array);
			$j = 0;
			$texts_size = count($texts_file_array);
			$k = 0;
			$followers_size = count($followers_file_array);

			// Loop through the arrays of the file content
			// If we reach the end of the array or the line was empty, print empty table data
			// If there is info to display, call the function that will display the info
			while( ($i < $followees_size) or ($j < $texts_size) or ($k < $followers_size) ) {
				echo "<tr>";
				if($i >= $followees_size) {
					blank_table_data("left", "28", "2", "2", "2", "1");
				}
				elseif($followees_file_array[$i] == "") {
					blank_table_data("left", "28", "2", "2", "2", "1");
				}
				else {
					// $followees_data will contain two values: position 0 = followee's index, position 1 = followee's username
					$followees_data = explode(",", $followees_file_array[$i]);
					echo "<td align='left' width='28%' style='padding: 2px 2px 2px 1px'>";
					other_user_table_data($followees_data[0], $followees_data[1], TRUE, "middle");
					echo "</td>";
				}

				if($j >= $texts_size) {
					blank_table_data("center", "44", "2", "2", "2", "2");
				}
				elseif($texts_file_array[$j] == "") {
					blank_table_data("center", "44", "2", "2", "2", "2");
				}
				else {
					echo "<td align='center' width='44%' style='padding: 2px 2px 2px 2px'>";
					texty_table_data($texts_file_array[$j], 2, 60);
					echo "</td>";
				}
				
				if($k >= $followers_size) {
					blank_table_data("right", "28", "2", "2", "2", "1");
				}
				elseif($followers_file_array[$k] == "") {
					blank_table_data("right", "28", "2", "2", "2", "1");
				}
				else {
					// $followers_data will contain two values: position 0 = follower's index, position 1 = follower's username
					$followers_data = explode(",", $followers_file_array[$i]);
					echo "<td align='right' width='28%' style='padding: 2px 1px 2px 2px'>";
					other_user_table_data($followers_data[0], $followers_data[1], FALSE, "top");
					echo "</td>";
				}
				echo "</tr>";
				$i++; $j++; $k++;				
			}




		?>

	</table>

	<hr/>

	<p>
		<a href="deactivate.html" align="left"><u>Deactivate Account</u></a>
	</p>
	<br/>

	<!-- Texty's hidden egg -->
	<p align="center"><a href="origin.html" style="font-size:70px; color: rgb(175, 50, 111)"><b>Texty</b>: The Origin</a></p>


	<!-- PHP Functions -->
	<?php

		// other_user_table_data takes in the index and username of someone that is not the logged in user along with a boolean.
		// It will display a link to another php file that will generate the other user's info in the same format as the homepage without the option to submit a texty.
		// If $cond is true, that means the logged in user follows this person so will display their most recent texty under their link.
		function other_user_table_data($index, $username, $cond, $vert_align) {
			// Display a link to view the other user's page
			echo "<a href='other_user.php?username={$username}' style='vertical-align: {$vert_align}'>{$username}</a>";
			// If $cond is true, we will also want to display their recent texty since we follow them.
			if($cond) {
				echo "<br/>"; /* line break so texty is below the other user's username link */
				$recent_texty = talk_to_server_recent_texty($username);
				// If there's no texty's to read, then don't display anything...
				if ($recent_texty == "\n") {
					// don't display anything
				}
				else {
					texty_table_data($recent_texty, 3, 40);
					// echo "<textarea rows='3' cols='40' disabled='disabled'>$line</textarea>";
				}
			}

		}

		// texty_table_data takes a line from someone's texts.txt file and displays it with a passed in row and column size
		function texty_table_data($texty_line, $row_size, $col_size) {
			echo "<textarea rows='{$row_size}' cols='{$col_size}' disabled='disabled'>{$texty_line}</textarea>";
		}

		// blank_table_data takes in an alignment of left, center, or right along with a percentage of how much page width it will occupy
		// It also takes the top, right, bottom, and left padding values to surround this table data with
		function blank_table_data($alignment, $width_percent, $top, $right, $bottom, $left) {
			echo "<td align='{$alignment}' width='{$width_percent}%' style='padding: {$top}px {$right}px {$bottom}px {$left}px'></td>";
		}

		// If $fol_unfol is "follow": go to user's followee file and add other user & go to other user's follower file and add user
		// If $fol_unfol is "unfollow": go to user's followee file and remove other user & go to other user's follower file and remove user
		function submit_fol_unfol($username, $other_username, $fol_unfol) {
			// $user_file = "/var/www/html/files/" . $user_index . "/followees.txt";			
			// $other_user_file = "/var/www/html/files/" . $other_user_index . "/followers.txt";
			if($fol_unfol == "follow") {
				talk_to_server_follow($username, $other_username);
			}
			else {
				talk_to_server_unfollow($username, $other_username);
			}
		}


		/*
		passes a message to the server with a function to call on the server and user data;
		attempts to received a message back from the server which will indicate success or failure of logging in
		*/
		function talk_to_server_display($un) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = DISPLAY_PAGE_FUNC . "," . $un . ",,,,";
			$message = append_commas($message); // append commas until message's length is PHP_MSG_SIZE
			fwrite($socket_fd, $message);
			$received_message;
			stream_set_blocking($socket_fd, 1); // Blocks the stream until data is available on it
			$received_message = stream_get_contents($socket_fd);
			fclose($socket_fd);
			return $received_message;
		}

		// same as above talk to server but this one handles getting a user's most recent texty
		function talk_to_server_recent_texty($un) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = LAST_TEXTY_FUNC . "," . $un;
			$message = append_commas($message); // append commas until message's length is PHP_MSG_SIZE
			fwrite($socket_fd, $message);
			$received_message;
			stream_set_blocking($socket_fd, 1); // Blocks the stream until data is available on it
			$received_message = stream_get_contents($socket_fd);
			fclose($socket_fd);
			return $received_message;
		}

		// same as above talk to server but this one handles following
		function talk_to_server_follow($un, $other_un) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = FOLLOW_FUNC . "," . $un . "," . $other_un;
			$message = append_commas($message); // append commas until message's length is PHP_MSG_SIZE
			fwrite($socket_fd, $message);
			$received_message;
			stream_set_blocking($socket_fd, 1); // Blocks the stream until data is available on it
			$received_message = stream_get_contents($socket_fd);
			fclose($socket_fd);
			return $received_message;
		}

		// same as above talk to server but this one handles unfollowing
		function talk_to_server_unfollow($un, $other_un) {
			// loop until a socket connection is created
			do {
				$socket_fd = stream_socket_client('localhost:'.get_random_port(), $errno, $errstr, 25);
			} while (!$socket_fd);
			
			// $message is the string that will be sent over the open socket to the server
			// It will be in the format: function_name,username,email,password,firstname,lastname
			// String will delimit different data fields by commas, even if a data field is empty
			$message = UNFOLLOW_FUNC . "," . $un . "," . $other_un;
			$message = append_commas($message); // append commas until message's length is PHP_MSG_SIZE
			fwrite($socket_fd, $message);
			$received_message;
			stream_set_blocking($socket_fd, 1); // Blocks the stream until data is available on it
			$received_message = stream_get_contents($socket_fd);
			fclose($socket_fd);
			return $received_message;
		}
	
	?>
	<!-- END of PHP Functions -->


</body>
</html>
