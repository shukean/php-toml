--TEST--
Check for toml table
--SKIPIF--
<?php if (!extension_loaded("toml")) print "skip"; ?>
--FILE--
<?php 
echo json_encode(toml_parse_file(__DIR__.'/006.toml'));
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
--EXPECT--
{"table":{"1":{"key1":"some string","key2":123},"2":{"key1":"another string","key2":456}}}