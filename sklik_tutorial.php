<?php

// Sklik username
$sklikUserName = "";
// Sklik password
$sklikPassword = "";
// True to debug using sandbox instead of production API
$useSandbox = false;

include 'xmlrpc.inc'; // ! Fixed XMLRPC lib
include 'sklik.inc'; // Sklik lib

try {
	$sklik = new Sklik($sklikUserName, $sklikPassword, $userSandbox);

	// Retrieve my account information
	$res = $sklik->client->get();
	print_r($res);

	// Retrieve statistics of campaigns
	$res = $sklik->campaigns->stats(
		array(590451, 590451),
		array(
			"dateFrom" => Sklik::dateTime(mktime(0, 0, 0, 1, 1, 2012)),
			"dateTo" => Sklik::dateTime(mktime(0, 0, 0, 1, 31, 2012)),
			"granularity" => "daily",
			"includeFulltext" => true,
			"includeContext" => true
		)
	);
	print_r($res);

	// Manage foreign accounts
	$sklik->setUserId(123456);
	$res = $sklik->campaigns->list();
	print_r($res);

// If method call fails
} catch (MethodError $e) {
	echo $e; // Returns error description including traceback

	print_r($e->getResponse()); // Result returned from Sklik API
}

