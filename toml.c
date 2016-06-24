/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: yukeyong87@gmail.com                                         |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_toml.h"
#include "zend_strtod.h"
#include "ext/standard/php_string.h"

/* If you declare any globals in php_toml.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(toml)
*/

/* True global resources - no need for thread safety here */
static int le_toml;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("toml.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_toml_globals, toml_globals)
    STD_PHP_INI_ENTRY("toml.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_toml_globals, toml_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

#define BUFFER_COPY_RESIZE(str)                                             \
    do{                                                                     \
        char _ch = (char) str;                                              \
        if (buffer_used + sizeof(_ch) >= buffer_length){                    \
            buffer_length += buffer_used + sizeof(_ch) * TOML_BUFFER_SIZE;  \
            buffer = erealloc(buffer, buffer_length);                       \
        }                                                                   \
        memcpy(buffer + buffer_used, &_ch, sizeof(_ch));                    \
        buffer_used += sizeof(_ch);                                         \
    }while(0)                                                               \

#define CHECK_NOT_WHITESPACE(c) \
    ((c) == '\b' || (c) == '\t' || (c) == '\n' || (c) == '\f' || (c) == '\r' || (c) == ' ') \


static char* big_numeral(char *str){
    char *buf, ch;
    size_t used = 0;
    
    buf = (char *)emalloc(sizeof(char) * strlen(str));
    memset((void *)buf, 0, strlen(str));
    
    while (*str) {
        ch = *str;
        if (!(*str == '_' || *str == ',')){
            memcpy(buf + used, &ch, sizeof(ch));
            used += sizeof(ch);
        }
        str++;
    }
    return buf;
}

static zend_bool is_float(char *str){
    
    if (*str > '9'){
        return 0;
    }
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str) {
        if (!((*str >= '0' && *str <= '9') || *str == '.' || *str == '_' || *str == ','
              || *str == 'e' || *str == 'E' || *str == '-' || *str == '+')){
            return 0;
        }
        str++;
    }
    return 1;
}

static zend_bool is_int(char *str){
    
    if (*str > '9'){
        return 0;
    }
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str) {
        if (!((*str >= '0' && *str <= '9') || *str == '_' || *str == ',')){
            return 0;
        }
        str++;
    }
    return 1;
}

static zval toml_parse_value(char *item_value, int line){
    zval default_val;
//    printf("%s--%d\n", item_value, strlen(item_value));
    
    if (!memcmp(item_value, "true", strlen(item_value)) || !memcmp(item_value, "false", strlen(item_value)) ) {
        zval b;
        ZVAL_BOOL(&b, !memcmp(item_value, "true", strlen(item_value)));
        return b;
    }

    if (is_int(item_value)) {
        zval i;
        char *new_str;
        
        new_str = big_numeral(item_value);
        ZVAL_LONG(&i, atol(new_str));
        efree(new_str);
        return i;
    }

    if (is_float(item_value)) {
        zval f;
        char *new_str;
        
        new_str = big_numeral(item_value);
        ZVAL_DOUBLE(&f, zend_strtod(new_str, NULL));
        efree(new_str);
        return f;
    }

    if ((item_value[0] == '"' && item_value[strlen(item_value) - 1] == '"')
        || (item_value[0] == '\'' && item_value[strlen(item_value) - 1] == '\'')) {
        zval str;
        
        if (strlen(item_value) == 2) {
            ZVAL_EMPTY_STRING(&str);
        }else{
            ZVAL_STRINGL(&str, item_value + 1, strlen(item_value) - 2);
            if(item_value[0] == '"'){
                php_stripcslashes(Z_STR(str));
            }
        }
        return str;
    }

    if (item_value[0] == '[' && item_value[strlen(item_value) -1] == ']') {
        return toml_parse_array(item_value, line);
    }
    
    php_error_docref(NULL, E_ERROR, "Undefined type not supported %s, line %d", item_value, line);
    
    ZVAL_NULL(&default_val);
    return default_val;
}

static zval toml_parse_array(char *item_value, int line){
	zval arr;
	size_t i, len = strlen(item_value) - 1;
	int depth = 0;
	zend_bool in_string = 0;
	char *buffer = NULL;
	size_t buffer_length = sizeof(char) * TOML_BUFFER_SIZE + 1, buffer_used = 0;

	buffer = (char *) emalloc(buffer_length);
	memset((void *)buffer, 0, buffer_length);

	array_init(&arr);
	for (i = 1; i<len; i++) {
		char ch = item_value[i];
		if (ch == '[') {
			depth++;
		}
		if (ch == ']') {
			depth--;
		}

		if ((ch == '"' && item_value[i-1] != '\\')) {
			in_string = in_string ? 0 : 1;
		}

		if (!in_string && ch == ',' && 0 == depth) {
            zval val;
			val = toml_parse_value(buffer, line);
            zend_hash_next_index_insert(Z_ARR(arr), &val);

			memset((void *)buffer, 0, buffer_length);
			buffer_used = 0;
			continue;
		}
		BUFFER_COPY_RESIZE(ch);
	}

	if (depth != 0) {
		php_error_docref(NULL, E_ERROR, "Unclosed array %s, line %d", item_value, line);
		return arr;
	}

    if (strlen(buffer) > 0) {
        zval val;
        val = toml_parse_value(buffer, line);
        zend_hash_next_index_insert(Z_ARR(arr), &val);
    }

	efree(buffer);
    return arr;
}

static void toml_parse_line(zval *result, zval **group_add_item, char *org_row, int line){
    size_t len, i;
    char *item_key, *item_value;
    zval *group, *found_group;
    char *row = org_row;
    
    len = strlen(row) - 1;
    if (len == -1) {
        return;
    }

    //try to parse group
    if(row[0] == '[' && row[len] == ']'){
        char *tmp_str = NULL, *tmp_stock_str = NULL;
        char *group_key;
        zval group_value;
        zend_bool is_array_table = 0;
        
        if (row[1] == '[' && row[len-1] == ']') {
            is_array_table = 1;
            tmp_str = estrndup(row + 2, len-3);
        }else{
            tmp_str = estrndup(row + 1, len-1);
        }
        
        group_key = php_strtok_r(tmp_str, ".", &tmp_stock_str);
        if (!group_key) {
            efree(tmp_str);
            php_error_docref(NULL, E_ERROR, "Group name parse fail : %s, line: %d", row, line);
            return;
        }
        
        group = zend_hash_str_find(Z_ARRVAL_P(result), group_key, strlen(group_key));
        if (!group){
            array_init(&group_value);
            if (!is_array_table) {
                group = zend_hash_str_update(Z_ARRVAL_P(result), group_key, strlen(group_key), &group_value);
            }else{
                zval tmp_group_value;
                uint32_t num;

                array_init(&tmp_group_value);
                found_group = zend_hash_str_update(Z_ARRVAL_P(result), group_key, strlen(group_key), &group_value);
                zend_hash_next_index_insert(Z_ARR_P(found_group), &tmp_group_value);
                
                num = zend_hash_num_elements(Z_ARR_P(found_group));
                group = zend_hash_index_find(Z_ARR_P(found_group), num - 1);
                if (!group) {
                    php_error_docref(NULL, E_ERROR, "Array tables core error");
                }
            }
        }else{
            if (!is_array_table) {
                if (tmp_stock_str == NULL) {
                    php_error_docref(NULL, E_ERROR, "Table %s is alreay defind, line %d. ", group_key, line);
                    return;
                }
            }else{
                zval tmp_group_value;
                uint32_t num;
                
                if (tmp_stock_str == NULL) {
                    array_init(&tmp_group_value);
                    zend_hash_next_index_insert(Z_ARR_P(group), &tmp_group_value);
                }

                num = zend_hash_num_elements(Z_ARR_P(group));
                group = zend_hash_index_find(Z_ARR_P(group), num - 1);
                if (!group) {
                    php_error_docref(NULL, E_ERROR, "Array tables core error");
                }
            }
        }
        
        while ((group_key = php_strtok_r(NULL, ".", &tmp_stock_str))) {
            found_group = zend_hash_str_find(Z_ARRVAL_P(group), group_key, strlen(group_key));
            if (!found_group){
                if (!is_array_table) {
                    zval group_child_value;
                    array_init(&group_child_value);
                    found_group = zend_hash_str_update(Z_ARRVAL_P(group), group_key, strlen(group_key), &group_child_value);
                }else{
                    zval group_child_value;
                    array_init(&group_child_value);
                    found_group = zend_hash_str_update(Z_ARRVAL_P(group), group_key, strlen(group_key), &group_child_value);
                }
            }else{
                if (!is_array_table && tmp_stock_str == NULL) {
                    php_error_docref(NULL, E_ERROR, "Table %s is alreay defind, line %d. ", group_key, line);
                    return;
                }
            }
            
            if (!is_array_table) {
                group = found_group;
            }else{
                zval tmp_group_value;
                uint32_t num;
                
                if (tmp_stock_str == NULL) {
                    array_init(&tmp_group_value);
                    zend_hash_next_index_insert(Z_ARR_P(found_group), &tmp_group_value);
                }
                
                num = zend_hash_num_elements(Z_ARR_P(found_group));
//                printf("%d", num);
                group = zend_hash_index_find(Z_ARR_P(found_group), num - 1);
                if (!group) {
                    php_error_docref(NULL, E_ERROR, "Array tables core error");
                }
            }
        }
        efree(tmp_str);
        *group_add_item = group;
        return;
    }

    item_key = php_strtok_r(row, "=", &item_value);
    if (item_key) {
        zval parse_value;

        if (!*group_add_item) {
            group = result;
        }else{
            group = *group_add_item;
        }
        
        //blank key
        if (!item_value) {
            item_value = item_key;
            item_key = NULL;
        }else if(strlen(item_key) == 2 &&
                 ((item_key[0] == '\'' && item_key[1] == '\'') || (item_key[0] == '"' && item_key[1] == '"'))){
            item_key = NULL;
        }

        parse_value = toml_parse_value(item_value, line);
        if (item_key == NULL) {
            zend_hash_next_index_insert(Z_ARR_P(group), &parse_value);
        }else{
            zend_hash_str_update(Z_ARR_P(group), item_key, strlen(item_key), &parse_value);
        }

    }else{
        php_error_docref(NULL, E_ERROR, "Invalid key: %s, lime %d", row, line);
    }


}


/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_toml_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(toml_parse)
{
	zend_string *toml_file;
    zval result, *group = NULL;
    php_stream *stream;
    zend_bool in_string = 0, in_comment = 0;
    zend_bool in_basic_string = 0,  in_literal_string = 0;
    zend_bool in_multi_basic_string = 0, in_multi_literal_string = 0;
    zend_bool ignore_blank_to_next_char = 0;
    unsigned int array_depth = 0;
    zend_string *toml_contents;
    char *buffer = NULL, *contents;
    size_t buffer_length = sizeof(char) * TOML_BUFFER_SIZE + 1, buffer_used = 0;
    int line = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &toml_file) == FAILURE) {
		return;
	}

    stream = php_stream_open_wrapper(ZSTR_VAL(toml_file), "rb", USE_PATH | REPORT_ERRORS, NULL);
    if (!stream) {
        php_error_docref(NULL, E_ERROR, "toml file open fail");
        RETURN_FALSE;
    }
    
    if ((toml_contents = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0)) == NULL) {
        php_stream_close(stream);
        php_error_docref(NULL, E_NOTICE, "toml file empty");
        array_init(return_value);
        return;
    }

    buffer = (char *) emalloc(buffer_length);
    memset((void *)buffer, 0, buffer_length);

    array_init(&result);

    contents = ZSTR_VAL(toml_contents);
    for (size_t i=0, len=ZSTR_LEN(toml_contents); i<len; i++) {
        int input_char = contents[i];

        //start of comment
        if (input_char == '#' && !in_string) {
            in_comment = 1;
        }

//        printf("%c", input_char);
        // start / end of string
        if (input_char == '"' && (i == 0 || contents[i-1]!= '\\') && !in_literal_string && !in_multi_literal_string) {
            in_string = in_string ? 0 : 1;
            if (contents[i+1] == '"' && contents[i+2] == '"') {
                if (in_basic_string) {
                    php_error_docref(NULL, E_ERROR, "Basic string can't content multi string flag, line %d", line);
                    RETURN_FALSE;
                }
                in_multi_basic_string = in_multi_basic_string ? 0 : 1;
                i+=2;
                if (in_multi_basic_string && contents[i+1] == '\n') {
                    line++;
                    i+=1;
                }
                BUFFER_COPY_RESIZE(input_char);
                continue;
            }else{
                if (in_multi_basic_string) {
                    php_error_docref(NULL, E_ERROR, "Basic multi string can't content single flag, line %d", line);
                    RETURN_FALSE;
                }
                in_basic_string = in_basic_string ? 0 : 1;
            }
        }
        // start / end of string
        if (input_char == '\'' && (i == 0 || contents[i-1]!= '\\') && !in_basic_string && !in_multi_basic_string) {
            if (contents[i+1] == '\'' && contents[i+2] == '\'') {
                in_string = in_string ? 0 : 1;
                if (in_literal_string) {
                    php_error_docref(NULL, E_ERROR, "Literal string can't content multi literal string flag, line %d", line);
                    RETURN_FALSE;
                }
                in_multi_literal_string = in_multi_literal_string ? 0 : 1;
                i+=2;
                if (in_multi_literal_string && contents[i+1] == '\n') {
                    line++;
                    i+=1;
                }
                BUFFER_COPY_RESIZE(input_char);
                continue;
            }else{
                if (!in_multi_literal_string) {
                    in_string = in_string ? 0 : 1;
                    in_literal_string = in_literal_string ? 0 : 1;
                }
            }
        }

        if (input_char == '[' && !in_string) {
            array_depth ++;
        }

        if (input_char == ']' && !in_string) {
            array_depth --;
        }

        if (input_char == ' ' && !in_string) {
            continue;
        }

        if (in_multi_basic_string) {
            ignore_blank_to_next_char = 0;
            if (contents[i] == '\\' && contents[i+1] == '\n') {
                ignore_blank_to_next_char = 1;
                for (; ignore_blank_to_next_char; i++) {
                    if (contents[i] == '\n') {
                        line++;
                    }
                    if (!CHECK_NOT_WHITESPACE(contents[i+1])) {
                        ignore_blank_to_next_char = 0;
                    }
                }
                i--;
                continue;
            }
            if (!ignore_blank_to_next_char && contents[i] == '\n') {
                line++;
                BUFFER_COPY_RESIZE('\n');
                continue;
            }
        }
        if (in_multi_literal_string){
            if (contents[i] == '\n') {
                line++;
                BUFFER_COPY_RESIZE('\n');
                continue;
            }
        }
        
        if (input_char == '\n') {
            in_comment = 0;

            if (in_string) {
                php_error_docref(NULL, E_ERROR, "String exception: found a linefeed char, line: %d", line);
            }

            if (array_depth == 0) {
//                printf("%s\n", buffer);
                toml_parse_line(&result, &group, buffer, line);
                memset((void *)buffer, 0, buffer_length);
                buffer_used = 0;
            }
            
            line ++;
            continue;
        }

        if (in_comment) {
            continue;
        }

        BUFFER_COPY_RESIZE(input_char);

    };

    toml_parse_line(&result, &group, buffer, line + 1);

    efree(buffer);
    zend_string_free(toml_contents);
    php_stream_close(stream);

    ZVAL_COPY_VALUE(return_value, &result);

    return;
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_toml_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_toml_init_globals(zend_toml_globals *toml_globals)
{
	toml_globals->global_value = 0;
	toml_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(toml)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(toml)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(toml)
{
#if defined(COMPILE_DL_TOML) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(toml)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(toml)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "toml support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ toml_functions[]
 *
 * Every user visible function must have an entry in toml_functions[].
 */
const zend_function_entry toml_functions[] = {
	PHP_FE(toml_parse,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in toml_functions[] */
};
/* }}} */

/* {{{ toml_module_entry
 */
zend_module_entry toml_module_entry = {
	STANDARD_MODULE_HEADER,
	"toml",
	toml_functions,
	PHP_MINIT(toml),
	PHP_MSHUTDOWN(toml),
	PHP_RINIT(toml),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(toml),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(toml),
	PHP_TOML_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_TOML
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(toml)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
