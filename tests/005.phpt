--TEST--
Check for toml array
--SKIPIF--
<?php if (!extension_loaded("toml")) print "skip"; ?>
--FILE--
<?php 
echo json_encode(toml_parse(__DIR__.'/005.toml'));
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
{"arr1":[1,2,3],"arr2":["red","yellow","green"],"arr3":[[1,2],[3,4,5]],"arr4":["all","strings","\"\"are the same\"\"","''type''"],"arr5":[[1,2],["a","b","c"]],"arr6":[1,2],"arr7":[1,2,3],"arr8":[1,2]}