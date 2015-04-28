<!-- JOSEPH BIESELIN - Texty Web App -->
<?php
	session_start();
	session_unset();
	session_destroy();
	// If index.php is loaded, a user logged out or someone just registerd so we must unset all session variables and destroy the session
?>

<!-- index.php is equivalent to index.html aside from php's session handling at the top of this file -->
<!DOCTYPE html>
<html>
<head>
	<title>texty</title>
</head>
<body>

	<h1>You've reached texty!</h1>

	<!-- Javascript for form validation -->
	<script>

		function check_signin() {

		var username = document.getElementById("username").value;
		var password = document.getElementById("password").value;
		
			if (username.indexOf(",") != -1 || password.indexOf(",") != -1) {
				alert("Can't submit with ',' in any input fields");
				return false;
			}

			if (username.indexOf(" ") != -1) {
				alert("Can't submit with spaces in the username");
				return false;
			}

			if (username.length > 16) {
				alert("Username is limited to 16 characters");
				return false;
			}

			if (password.length > 16) {
				alert("Password is limited to 16 characters");
				return false;
			}

			alert('Data being submitted');
			return true;
		}

	</script>

	<form onsubmit="return check_signin()" method="post" action="check_user.php" autocomplete="on">
		<fieldset>
			<p>
				<table>
					<tr>
						<th align="left">
							Username:&nbsp;<input type="text" name="username" placeholder="Enter your username here" id="username" required>
						</th>
						<th align="right">
							Password:&nbsp;<input type="password" name="password" placeholder="Enter your password here" id="password" required>
						</th>
						<th align="center">
							<input type="submit" value="Sign In">
						</th>
					</tr>
					<tr>
						<td></td>
						<td></td>
						<td align="center">
							<a href="register.html" style="color:rgb(180,150,20)"><u>Register</u></a>
						</td>
					</tr>
				</table>
			<p>
		</fieldset>
	</form>

	<p align="center" style="font-size:100px"><u><b>texty</b></u></p>
	<p align="center">Take what you want to say and put it on the internet forever</p>

	<hr/>

	<a href="deactivate.html"><u>Deactivate Account</u></a>

</body>
</html>