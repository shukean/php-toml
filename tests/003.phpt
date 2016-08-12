--TEST--
Check for toml float
--SKIPIF--
<?php if (!extension_loaded("toml")) print "skip"; ?>
--FILE--
<?php 
echo json_encode(toml_parse_file(__DIR__.'/003.toml'));
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
{"flt1":1,"flt2":3.1415,"flt3":-0.01,"flt4":5.0e+22,"flt5":1000000,"flt6":-0.02,"flt7":6.626e-34}