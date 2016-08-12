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


#define CHECK_NOT_WHITESPACE(c) \
    ((c) == '\b' || (c) == '\t' || (c) == '\n' || (c) == '\f' || (c) == '\r' || (c) == ' ') 


#define TOML_PARSE_STR(str, str_len) toml_parse_str(str, str_len, &result, &group, &top_is_array_table, line)


#define COPY_CHAR_TO_BUF(c, buf, pos){              \
    memcpy(buf + pos, &c, sizeof(c));               \
    pos += sizeof(c);                               \
}


static zend_uchar toml_is_numeric(char *str, zend_long *lval, double  *dval){
    size_t s1 = 0, s2 = 0, l = 0;
    char *new = ecalloc(sizeof(char), strlen(str) + 1);
    char *raw = str;
    zend_uchar ret;
    
    while (*str) {
        if (*str == '_' || *str == ','){
            memcpy(new + s2, raw + s1, l);
            s1 += l + 1;
            s2 += l;
            l = 0;
        }else{
            l++;
        }
        str++;
    }
    memcpy(new + s2, raw + s1, l);
//    printf("%s\n", new);
    ret = is_numeric_string(new, strlen(new), lval, dval, 1);
    efree(new);
    return ret;
}

static zval toml_parse_item_array(char *item_value, size_t max_len, int line){
	zval arr;
	size_t i, len = strlen(item_value) - 1;
	int depth = 0;
	zend_bool in_string = 0;
	char *buffer = NULL;
	size_t buffer_used = 0;

    buffer = ecalloc(sizeof(char), max_len + 1);

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
			val = toml_parse_item_value(buffer, buffer_used, line);
            zend_hash_next_index_insert(Z_ARR(arr), &val);

			memset((void *)buffer, 0, max_len + 1);
			buffer_used = 0;
			continue;
		}
        COPY_CHAR_TO_BUF(ch, buffer, buffer_used);
	}

	if (depth != 0) {
		php_error_docref(NULL, E_ERROR, "Unclosed array %s, line %d", item_value, line);
		return arr;
	}

    if (buffer_used > 0) {
        zval val;
        val = toml_parse_item_value(buffer, buffer_used, line);
        zend_hash_next_index_insert(Z_ARR(arr), &val);
    }

	efree(buffer);
    return arr;
}

static zval* get_array_last_item(zval *stack){
    zval *val;
    uint32_t idx;
    Bucket *p;
    
    /* Get the last value and copy it into the return value */
    idx = Z_ARRVAL_P(stack)->nNumUsed;
    while (1) {
        if (idx == 0) {
            return NULL;
        }
        idx--;
        p = Z_ARRVAL_P(stack)->arData + idx;
        val = &p->val;
        break;
    }
    return val;
}


static zval toml_parse_item_value(char *item_value, size_t max_len, int line){
    zval val;
    size_t item_val_len = strlen(item_value);
    
    int  ret_true;
    
    zend_uchar ret_numeric;
    zend_long lval;
    double dval;
    
//    printf("%s--%lu\n", item_value, strlen(item_value));
    
    ret_true = memcmp(item_value, "true", strlen(item_value)) == 0;
    if (ret_true|| memcmp(item_value, "false", strlen(item_value)) == 0) {
        ZVAL_BOOL(&val, ret_true);
        return val;
    }
    
    if ((ret_numeric = toml_is_numeric(item_value, &lval, &dval)) != 0) {
        if (ret_numeric == IS_LONG) {
            ZVAL_LONG(&val, lval);
        }else if (ret_numeric == IS_DOUBLE){
            ZVAL_DOUBLE(&val, dval);
        }else{
            php_error_docref(NULL, E_ERROR, "Convert to number fail %s, line %d", item_value, line);
        }
        return val;
    }
    
    
    if ((item_value[0] == '"' && item_value[item_val_len - 1] == '"')
        || (item_value[0] == '\'' && item_value[item_val_len - 1] == '\'')) {
        if (strlen(item_value) == 2) {
            ZVAL_EMPTY_STRING(&val);
        }else{
            ZVAL_STRINGL(&val, item_value + 1, item_val_len - 2);
            if(item_value[0] == '"'){
                php_stripcslashes(Z_STR(val));
            }
        }
        return val;
    }
    
    if (item_value[0] == '[' && item_value[item_val_len -1] == ']') {
        return toml_parse_item_array(item_value, max_len, line);
    }
    
    php_error_docref(NULL, E_ERROR, "Undefined value type not supported %s, line %d", item_value, line);
    
    ZVAL_NULL(&val);
    return val;
}


static void toml_parse_str(char *raw_str, size_t len, zval *result, zval **group, zend_bool *top_is_array_table, int line){
    char *item_key, *item_val;
    zval *g, *gp;
    char *str = raw_str;

//    printf("%s\n", str);
    //try to parse group
    if(str[0] == '[' && str[len - 1] == ']'){
        char *g_key_str, *g_tok_key_str;
        char *g_key;
        zend_bool is_array_table = 0;
        
        if (str[1] == '[' && str[len-2] == ']') {
            is_array_table = 1;
            g_key_str = estrndup(str + 2, len-4);
        }else{
            g_key_str = estrndup(str + 1, len-2);
        }
        
        g_key = php_strtok_r(g_key_str, ".", &g_tok_key_str);
        if (UNEXPECTED(!g_key)) {
            efree(g_key_str);
            php_error_docref(NULL, E_ERROR, "Group name parse fail : %s, line: %d", raw_str, line);
            return;
        }
        
        g = zend_hash_str_find(Z_ARRVAL_P(result), g_key, strlen(g_key));
        if (!g){
            zval items;
            array_init(&items);
            g = zend_hash_str_update(Z_ARRVAL_P(result), g_key, strlen(g_key), &items);
            
            if (is_array_table) {
                zval g_items;
                
                array_init(&g_items);
                g = zend_hash_next_index_insert(Z_ARR_P(g), &g_items);
                
                *top_is_array_table = 1;
            }else{
                *top_is_array_table = 0;
            }
        }else{
            if (!is_array_table && g_tok_key_str == NULL) {
                php_error_docref(NULL, E_ERROR, "Table %s is alreay defind, line %d. ", g_key, line);
                return;
            }
            
            if (is_array_table) {
                
                if (g_tok_key_str == NULL) {
                    zval g_items;
                    array_init(&g_items);
                    g = zend_hash_next_index_insert(Z_ARR_P(g), &g_items);
                }else{
                    g = get_array_last_item(g);
                }
            }else if (*top_is_array_table){
                g = get_array_last_item(g);
            }
        }
        
        while ((g_key = php_strtok_r(NULL, ".", &g_tok_key_str))) {
            gp = zend_hash_str_find(Z_ARRVAL_P(g), g_key, strlen(g_key));
            if (!gp){
                zval items;
                array_init(&items);
                gp = zend_hash_str_update(Z_ARRVAL_P(g), g_key, strlen(g_key), &items);
                
                if (is_array_table) {
                    zval g_items;
                    
                    array_init(&g_items);
                    gp = zend_hash_next_index_insert(Z_ARR_P(gp), &g_items);
                }
            }else{
                
                if (!is_array_table && g_tok_key_str == NULL) {
                    php_error_docref(NULL, E_ERROR, "Table %s is alreay defind, line %d. ", g_key, line);
                    return;
                }
                
                if (is_array_table) {
                    if (g_tok_key_str == NULL) {
                        zval g_items;
                        array_init(&g_items);
                        gp = zend_hash_next_index_insert(Z_ARR_P(gp), &g_items);
                    }else{
                        gp = get_array_last_item(gp);
                    }
                }else if (*top_is_array_table){
                    gp = get_array_last_item(gp);
                }
            }
            
            g = gp;
        }
        
        efree(g_key_str);
        *group = g;
        return;
    }

    
    item_key = php_strtok_r(str, "=", &item_val);
    if (item_key) {
        zval val;

        g = !*group ? result : *group;
        
        //blank key
        if (!item_val) {
            item_val = item_key;
            item_key = NULL;
        }else if(strlen(item_key) == 2 && (memcmp(item_key, "\"\"", 2) == 0 || memcmp(item_key, "''", 2) == 0)){
            item_key = NULL;
        }

        val = toml_parse_item_value(item_val, len, line);
        if (item_key == NULL) {
            zend_hash_next_index_insert(Z_ARR_P(g), &val);
        }else{
            zend_bool is_quoted_key = 0;
            if (item_key[0] == '"' || item_key[0] == '\'') {
                char *key = estrndup(item_key + 1, strlen(item_key) - 2);
                item_key = key;
                is_quoted_key = 1;
            }
            zend_hash_str_update(Z_ARR_P(g), item_key, strlen(item_key), &val);
            if (is_quoted_key) {
                efree(item_key);
            }
        }

    }else{
        php_error_docref(NULL, E_ERROR, "Invalid key: %s, lime %d", raw_str, line);
    }

}



PHP_FUNCTION(toml_parse_string)
{
    zend_string *toml_contents;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &toml_contents) == FAILURE) {
        return;
    }
    
    parse_toml(toml_contents, return_value);
}


PHP_FUNCTION(toml_parse_file)
{
    zend_string *toml_file, *toml_contents;
    php_stream *stream;
    
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
    
    php_stream_close(stream);
    
    parse_toml(toml_contents, return_value);
    
    zend_string_free(toml_contents);
}


static void parse_toml(zend_string *toml_contents, zval *return_value){
    zend_bool in_string = 0, in_comment = 0;
    zend_bool in_basic_string = 0,  in_literal_string = 0;
    zend_bool in_multi_basic_string = 0, in_multi_literal_string = 0;
    zend_bool ignore_blank_to_next_char = 0;
    unsigned int array_depth = 0;
    zval result, *group = NULL;
    zend_bool top_is_array_table = 0;
    char *buffer = NULL, *contents;
    size_t buffer_used = 0, toml_contents_len = 0;
    unsigned int line = 1;

    array_init(&result);
    toml_contents_len = ZSTR_LEN(toml_contents);
    buffer = (char *) ecalloc(sizeof(char), toml_contents_len + 1);
    contents = ZSTR_VAL(toml_contents);
    
    for (size_t i=0, len=ZSTR_LEN(toml_contents); i<len; i++) {
        char input_char = contents[i];

        //start of comment
        if (input_char == '#' && !in_string) {
            in_comment = 1;
        }
        
        //windows or mac
        if (input_char == '\r') {
            if (contents[i+1] == '\n') {
                i++;
                continue;
            }
            input_char = '\n';
        }
        
        if (input_char == '\n'){
            line++;
        }

//        printf("%c", input_char);
        // start / end of string
        if (input_char == '"' && (i == 0 || contents[i-1]!= '\\') && !in_literal_string && !in_multi_literal_string) {
            in_string = in_string ? 0 : 1;
            //multi string
            if(memcmp(contents + i + 1, "\"\"", 2) == 0){
//            if (contents[i+1] == '"' && contents[i+2] == '"') {
                if (UNEXPECTED(in_basic_string)) {
                    php_error_docref(NULL, E_ERROR, "Basic single string can not contain multi-string flag, line %d", line);
                    RETURN_FALSE;
                }
                in_multi_basic_string = in_multi_basic_string ? 0 : 1;
                i+=2;
                if (in_multi_basic_string && contents[i+1] == '\n') {
                    line++;
                    i+=1;
                }
            }else{
                if (UNEXPECTED(in_multi_basic_string)) {
                    php_error_docref(NULL, E_ERROR, "Basic multi-string can not contain single string flag, line %d", line);
                    RETURN_FALSE;
                }
                in_basic_string = in_basic_string ? 0 : 1;
            }
            COPY_CHAR_TO_BUF(input_char, buffer, buffer_used);
            continue;
        }
        
        // start / end of string
        if (input_char == '\'' && (i == 0 || contents[i-1]!= '\\') && !in_basic_string && !in_multi_basic_string) {
            if (memcmp(contents + i + 1, "''", 2) == 0) {
//            if (contents[i+1] == '\'' && contents[i+2] == '\'') {
                in_string = in_string ? 0 : 1;
                if (UNEXPECTED(in_literal_string)) {
                    php_error_docref(NULL, E_ERROR, "Literal string can't content multi literal string flag, line %d", line);
                    RETURN_FALSE;
                }
                in_multi_literal_string = in_multi_literal_string ? 0 : 1;
                i+=2;
                if (in_multi_literal_string && contents[i+1] == '\n') {
                    line++;
                    i+=1;
                }
            }else{
                if (!in_multi_literal_string) {
                    in_string = in_string ? 0 : 1;
                    in_literal_string = in_literal_string ? 0 : 1;
                }
            }
            COPY_CHAR_TO_BUF(input_char, buffer, buffer_used);
            continue;
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
            if (input_char == '\\' && contents[i+1] == '\n') {
                ignore_blank_to_next_char = 1;
                i += 1;
                continue;
            }
            
            if (ignore_blank_to_next_char) {
                if (CHECK_NOT_WHITESPACE(input_char)) {
                    continue;
                }
                ignore_blank_to_next_char = 0;
            }
        }
        
        if (input_char == '\n' && !(in_multi_literal_string || in_multi_basic_string)) {
            in_comment = 0;

            if (UNEXPECTED(in_string)) {
                php_error_docref(NULL, E_ERROR, "String exception: found a linefeed char, line: %d", line);
            }

            if (array_depth == 0 && buffer_used > 0) {
                TOML_PARSE_STR(buffer, buffer_used);
                memset((void *)buffer, 0, toml_contents_len + 1);
                buffer_used = 0;
            }
            
            continue;
        }

        if (in_comment) {
            continue;
        }

        COPY_CHAR_TO_BUF(input_char, buffer, buffer_used);

    };

    if(buffer_used > 0){
        TOML_PARSE_STR(buffer, buffer_used);
    }

    efree(buffer);
    
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
	PHP_FE(toml_parse_file,	NULL)
    PHP_FE(toml_parse_string, NULL)
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
