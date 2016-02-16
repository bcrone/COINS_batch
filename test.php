<?php
	$file = fopen('test.csv', 'r');
	while (($line = fgetcsv($file)) !== FALSE) {
		$input = array();
		foreach($line as $val) {
			$input[] = $val;
		}
		//foreach($input as $key => $value) {
			//echo $value.'<br>';
		//}
		$var1 = $input[0];
		$var2 = $input[1];
		$var3 = $input[2];
		$var4 = $input[3];
		$var5 = $input[4];
		echo '$var1 = '.$var1.'<br>';
		echo '$var2 = '.$var2.'<br>';
		echo '$var3 = '.$var3.'<br>';
		echo '$var4 = '.$var4.'<br>';
		echo '$var5 = '.$var5.'<br>';
		unset($input);
	}
	fclose($file);