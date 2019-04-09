<?php	
	header('Content-Type: text/html');
	
	$page = file_get_contents("mainpage.html");
	
	if (isset($_GET['cpp'])) {
		$page = str_replace("	", " ", $page);			
		$page = str_replace(PHP_EOL, " ", $page);
		$page = str_replace('"', '\"', $page);
		$page = str_replace("  ", " ", $page);			
		$page = str_replace("  ", " ", $page);			
		$page = str_replace("  ", " ", $page);		
		$page = str_replace("  ", " ", $page);		
		$page = str_replace("> <", "><", $page);			
	
	}
	else {
		$page = str_replace("{SSID_LIST}", "'SSID1', 'SSID2'", $page);
		$page = str_replace("{CURRENT_DEVICE}", "Remote #12345678", $page);
	}
	
	echo $page;
?>