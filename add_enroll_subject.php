<?php
require_once('../p2/AppData.inc');
$_SESSION['objDbLink'] = new AppData('mrs','admin');
$file = fopen('test.csv','r');
while (($line = fgetcsv($file)) !== FALSE){
	$input = array();
	foreach($line as $val){
		$input[] = $val;
	}
	
	$stype = $input[0];
	$BirthDate = $input[1];
	$site_id = $input[2];
	$LastName = $input[3];
	$Gender = $input[4];

	$_SESSION['stype'] = $stype;
	$array = array(
		"agreestosharedata" => "",
		"BirthDate" => $BirthDate,
		"site_id" => $site_id,
		"FirstName" => "DEFAULT",
		"MiddleName" => "",
		"LastName" => $LastName,
		"userEmail" => "",
		"context" => "",
		"subject_tag_id" => "",
		"Gender" => $Gender,
		"Line1" => "500 Newton Road",
		"Line2" => "",
		"City" => "Iowa City",
		"State" => "IA",
		"Zip" => "52242",
		"Country" => "USA",
		"Notes" => "",
		"Suffix" => "",
		"userPwd" => "",
		"first_name_at_birth" => "",
		"middle_name_at_birth" => "",
		"last_name_at_birth" => "",
		"physical_sex_at_birth" => "",
		"city_born_in" => "",
		"study_id" => "2239"
	);

	$_SESSION['newSubject'] = $array;
	$usid = $_SESSION['objDbLink']->addSubject($_SESSION['newSubject']);
	if ($usid === 'subjectExists') {
		$site = $_SESSION['objDbLink']->getSiteConfig($_SESSION['newSubject']['site_id']);
		$message = 'Subject ' . $_SESSION['newSubject']['LastName'] . ' already exists';
	} elseif ($usid === 'badEmail') {
		$site = $_SESSION['objDbLink']->getSiteConfig($_SESSION['newSubject']['site_id']);
		$message = 'Subject ' . $_SESSION['newSubject']['LastName'] . ' has improper email ' . $_SESSION['newSubject']['userEmail'];
	} elseif ($usid === 'emailExists') {
		$site = $_SESSION['objDbLink']->getSiteConfig($_SESSION['newSubject']['site_id']);
		$message = 'Subject ' . $_SESSION['newSubject']['LastName'] . ' has email already exists ' . $_SESSION['newSubject']['userEmail'];
	} else {
		echo '<p>Attempting to enroll</p>';
		$ursi = $_SESSION['objDbLink']->enrollSubject(
			$_SESSION['newSubject']['site_id'],
			$usid,
			$_SESSION['stype'],
			'',
			null,
			null,
			"01/01/2002",
			''
		);
		echo '<p>'.$message.'</p>';
		echo '<p>'.$ursi.'</p>';
	}
}
session_destroy();
die;