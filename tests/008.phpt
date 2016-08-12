--TEST--
Check for toml all
--SKIPIF--
<?php if (!extension_loaded("toml")) print "skip"; ?>
--FILE--
<?php 
echo json_encode(toml_parse(__DIR__.'/008.toml'));
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
{"name2":"AAA","name3":"BBB","int1":99,"int2":42,"int3":0,"int4":-17,"int5":1000,"int6":5349221,"int7":12345,"flt1":1,"flt2":3.1415,"flt3":-0.01,"flt4":5.0e+22,"flt5":1000000,"flt6":-0.02,"flt7":6.626e-34,"bool1":true,"bool2":false,"arr1":[1,2,3],"arr2":["red","yellow","green"],"arr3":[[1,2],[3,4,5]],"arr4":["all","strings","are the same","type"],"arr5":[[1,2],["a","b","c"]],"arr6":[1,2],"arr7":[1,2,3],"arr8":[1,2],"table":{"1":{"key1":"some string","key2":123},"2":{"key1":"another string","key2":456}},"name":{"first":"Tom","last":"Preston-Werner"},"point":{"x":1,"y":2}}