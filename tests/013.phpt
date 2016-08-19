--TEST--
Check for toml repeat group
--SKIPIF--
<?php if (!extension_loaded("toml")) print "skip"; ?>
--FILE--
<?php 
echo json_encode(toml_parse_file(__DIR__.'/013.toml'));
/*
	you can add regression tests for your extension here

  the output of your test code has to be equal to the
  text in the --EXPECT-- section below for the tests
  to pass, differences between the output and the
  expected text are interpreted as failure

	see php7/README.TESTING for further information on
  writing regression tests
*/
?>
--EXPECTF--
 Fatal error: Table group1 is alreay defind, line 7.  in %s/013.php on line %d