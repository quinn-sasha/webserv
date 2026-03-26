<?php
header("Content-Type: text/plain");

echo "This is PHP CGI Script \n";
echo "~~~~~~~~~~~~~~~~~~~~~~\n\n";

echo "Script Filename: " . $_SERVER['SCRIPT_FILENAME'] . "\n\n";
?>