# php-toml
A php7 extension for parse toml file

## Toml
https://github.com/toml-lang/toml   

##Methods

`array toml_parse(string toml_file_name) ` 
###Parameters
`toml_file_name`: Name of the file read.
###Return Values
The function returns array or **False** on failure.
###Errors/Exceptions
An E_ERROR level error is generated if toml contents parse fail, file cannot be found.
An E_NOTICE level error is generated if toml contents is empty.

##Toml Syntax Supported

###Comment
\# This is a full-line comment   
key = "value" # This is a comment at the end of a line

###Bool
a = true   
b = false   

*bool value must be lowercase letter*

###Key/Value Pair
key = "value"  
bare_key = "value"  
bare-key = "value"  
1234 = "value"  

"127.0.0.1" = "value"   
"character encoding" = "value"   
"ʎǝʞ" = "value"   
'key2' = "value"   
'quoted "value"' = "value"   

= "no key name"  # INVALID   
"" = "blank"     # VALID but discouraged   
'' = 'blank'     # VALID but discouraged   

*key not defind or empty, the key will replace to index num*

###Integer
int1 = +99   
int2 = 42   
int3 = 0   
int4 = -17   
int5 = 1_000   
int6 = 5_349_221   
int7 = 1_2_3_4_5   

*the underline also replace to a comma*

###Float
flt1 = +1.0   
flt2 = 3.1415   
flt3 = -0.01   
flt4 = 5e+22   
flt5 = 1e6   
flt6 = -2E-2   
flt7 = 6.626e-34
flt7 = 6.626e-34
flt8 = 9_224_617.445_991_228_313

*the underline also replace to a comma*  

###Datetime
**Not support**

###String

####Basic strings
name = "this a string"   

*value will be called php function __stripcslashes__*

####Multi-line basic strings
name = """   
Roses are red   
Violets are blue"""

\# the string will most likiey be the same as:  
name = "Roses are red\nViolets are blue"   

*For writing long strings without introducing extraneous whitespace, end a line with a \. The \ will be trimmed along with all whitespace (including newlines) up to the next non-whitespace character or closing delimiter.* 

name = """   
The qucik brown \
   
   for jumps over \   
   the lazy dog."""


####Literal strings
name2 = 'this a string too'  

####Multi-line literal strings
lines  = '''   
The first newline is   
trimmed in raw strings.   
   All other whitespace   
   is preserved.   
'''

*only char **\n** parsed* 

###Array
arr_a = ["str1", "str2"]   
arr_b = [   
   "str1",   
   "str2"   
]   
arr3 = [ [ 1, 2 ], [3, 4, 5] ]   
arr5 = [ [ 1, 2 ], ["a", "b", "c"] ]   

*same type is not necessary*


###Table

[group_a]   
key_a = "value"   

[group]   
  key_a = "value"   
  [group.a]   
  key_a = "value"  
  [group.b]   
  key_a = "value"  
  

**[dog."tater.man"] Not supported**

###Inline Table
**Not supported**


###Array of Tables
[[ab]]   
  name = "vvv"   
  [[ab.vv]]   
  name = "granny smith"   
  [[ab.vv]]   
  name = "granny smith 2"   

[[ab]]   
  name = "bbb"   
