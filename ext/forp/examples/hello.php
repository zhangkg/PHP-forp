<?php

$foo = new stdClass;
$foo->foo = 'foo';
$foo->bar = 'bar';

$var = array(0 => "my", "strkey" => "inspected", 3 => $foo);
forp_inspect('var', $var);
print_r(forp_dump());
