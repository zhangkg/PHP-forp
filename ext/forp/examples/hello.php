<?php

class foo {

		public $one;
		public $two;
		public $three;
		
		function __construct() {
				echo "hello";		
		}
}

$var = array(0 => "my", "strkey" => "inspected", 3 => new foo());
forp_inspect('var', $var);
print_r(forp_dump());
