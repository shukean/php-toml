# php-toml
A php7 extension for parse toml file

## Toml
https://github.com/toml-lang/toml   

##Methods

toml_parse(toml_file)  

return array   

##Contents

###bool
key_a = true   
key_b = false   

###int
int1 = +99   
int2 = 42   
int3 = 0   
int4 = -17   
int5 = 1_000   
int6 = 5_349_221   
int7 = 1_2_3_4_5   

###float
flt1 = +1.0   
flt2 = 3.1415   
flt3 = -0.01   
flt4 = 5e+22   
flt5 = 1e6   
flt6 = -2E-2   
flt7 = 6.626e-34   

###string
name = "this a string"   
name2 = 'this a string too'   

###array
arr_a = ["str1", "str2"]   
arr_b = [   
   "str1"   
   "str2"   
]   


###table

[group_a]   
key_a = "value"   

[group]   
  key_a = "value"   
  [group.a]   
  key_a = "value"  
  [group.b]   
  key_a = "value"  

