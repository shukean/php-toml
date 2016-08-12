# php-toml
A php7 extension for parse toml file

## Toml v0.4.0
https://github.com/toml-lang/toml   

## Methods

`array toml_parse_file(string toml_file_name) ` 
### Parameters
`toml_file_name`: Name of the file read.
### Return Values
The function returns array or **False** on failure.
### Errors/Exceptions
An E_ERROR level error is generated if toml contents parse fail, file cannot be found.   
An E_NOTICE level error is generated if toml contents is empty.


`array toml_parse_string(string toml_contents) ` 
### Parameters
`toml_contents`: A toml struct string.
### Return Values
The function returns array or **False** on failure.
### Errors/Exceptions
An E_ERROR level error is generated if toml contents parse fail.   
An E_NOTICE level error is generated if toml contents is empty.

## Toml Syntax Supported

### Comment
1. a full-line comment :  **yes**   
2. a comment at the end of a line :  **yes**   

### Bool
1. true / false  : **yes**   
   *bool value must be lowercase letter*

### Key/Value Pair
1. bare keys : **yes**   
2. quoted keys : **yes**     
3. empty keys: **yes**   
   *key not defind or empty, the key will replace to index num*

### Integer
1. normal : **yes**  
2. positive or negative : **yes**   
3. large : **yes**  
   
 *the value will be convert to php long*

### Float   
1. normal :  **yes**   
2. positive or negative : **yes**   
3. large : **yes** 
4. exponent : **yes**   
 *the value will be convert to php double* 

### Datetime
  **Not support**

### String

#### Basic strings
 **yes**   
 *value will be called php function __stripcslashes__*

#### Multi-line basic strings
 **yes**   
 extraneous whitespace : **yes**


#### Literal strings
 **yes**  

#### Multi-line literal strings
 **yes**

### Array
 **yes**   
 multiline : **yes**   
 data types fixed : **yes**  (^_^ php is best)   

### Table
1. normal : **yes**    
2. qoute key : **no**  (result is not you wanted)   
3. dots : **yes**   

### Inline Table
 **Not supported**

### Array of Tables
 **yes**  
