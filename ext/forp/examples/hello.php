<?php

class Foo {
  public $foo = 'foo';
	public $bar = 'bar';
}

$var = array("one" => "my", "strkey" => "inspected", "three" => new Foo());
forp_inspect('var', $var);
print_r(forp_dump());
