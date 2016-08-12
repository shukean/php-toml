--TEST--
Check for toml invalid key
--SKIPIF--
<?php if (!extension_loaded("toml")) print "skip"; ?>
--FILE--
<?php 
echo json_encode(toml_parse_file(__DIR__.'/014.toml'));
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
Fatal error: toml_parse_file(): Undefined value type not supported jjjjjjjjjj, line 3 in %s/tests/014.php on line %d
