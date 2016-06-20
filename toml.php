<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('toml')) {
	dl('toml.' . PHP_SHLIB_SUFFIX);
}
$module = 'toml';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach($functions as $func) {
    echo $func."$br\n";
}
echo "$br\n";


$ret = toml_parse('./demo.toml');

echo "\n";

print_r($ret);



?>
