 /*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2011 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Anthony Terrien <forp@anthonyterrien.com>                    |
  | Author: ____Shies Gukai <gukai@bilibili.com>                         |
  +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"

#include "forp.h"
#include "php_forp.h"

#include "forp_log.h"
#include "Zend/Zend_API.h"
#include "ext/standard/php_var.h"

#ifdef ZTS
#include "TSRM.h"
#endif

/* {{{ forp_zval_var
 */
forp_var_t *forp_zval_var(forp_var_t *v, zval *expr, int depth TSRMLS_DC) {

    char         s[32];
    int          is_tmp;
    zval         *tmp;
    zend_string  *key;
    zend_ulong   idx;
    ulong        max_depth;
    HashTable    *ht;
    const char   *resource_type;
    const char   *class_name, *prop_name;
    forp_var_t   **arr, *subarr;


    v->level   = NULL;
    v->value   = NULL;
    v->class   = NULL;
    v->arr     = NULL;
    v->arr_len = 0;


    v->is_ref = Z_ISREF_P(expr);
    if(Z_REFCOUNT_P(expr)>1) v->refcount = Z_REFCOUNT_P(expr);

    switch(Z_TYPE_P(expr)) {
        case IS_DOUBLE :
            /*
            sprintf(s, "%f", Z_DVAL_P(expr));
            v->value = strdup(s);
            v->type = "float";
            */
            break;
        case IS_LONG :
            /*
            sprintf(s, "%ld", Z_LVAL_P(expr));
            v->value = strdup(s);
            v->type = "int";
            */
            break;
        case IS_FALSE:
        case IS_TRUE:
            /*
            v->value = IS_TRUE ? "true" : "false";
            v->type = "bool";
            */
            break;
        case IS_STRING :
            /*
            v->type = "string";
            v->value = strdup(Z_STRVAL_P(expr));
            */
            break;
        case IS_ARRAY :
            v->type = "array";

            ht = Z_ARRVAL_P(expr);
            max_depth = FORP_G(inspect_depth_array);
            goto finalize_ht;
        case IS_OBJECT :
            /*
            v->type = "object";
            v->class = strdup(ZSTR_VAL(Z_OBJCE_P(expr)->name));
            sprintf(s, "#%d", Z_OBJ_HANDLE_P(expr));
            v->value = strdup(s);

            ht = Z_OBJDEBUG_P(expr, is_tmp);
            max_depth = FORP_G(inspect_depth_object);
            goto finalize_ht;
            */
finalize_ht:
            if (depth < max_depth + 1) {
                ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, tmp) {

                     arr = (forp_var_t **) realloc(v->arr, (v->arr_len+1) * sizeof(forp_var_t));
                     subarr = (forp_var_t *) malloc(sizeof(forp_var_t));
                     subarr->name = NULL;
                     subarr->stack_idx = -1;


                    if (key) {
                        if(strcmp(v->type, "object") == 0) {
                            size_t prop_name_len;
                            zend_unmangle_property_name_ex(key, &class_name, &prop_name, &prop_name_len);
                            if (class_name) {
                                subarr->type = strdup(class_name);
                                if (class_name[0] == '*') {
                                    subarr->level = "protected";
                                } else {
                                    subarr->level = "private";
                                }
                            } else {
                                subarr->level = "public";
                            }
                            subarr->key = strdup(prop_name);
                        } else {
                            subarr->key = strdup(ZSTR_VAL(key));
                        }
                    } else if (scanf("%lld", &idx)) {
                        sprintf(s, "%lu", idx);
                        subarr->key = strdup(s);
                    } else {
                        subarr->key = "*";
                    }

                    forp_zval_var(subarr, tmp, depth + 1 TSRMLS_CC);

                    // php_printf(
                    //        "%*s > %s\n", depth, "",
                    //        v->arr[v->arr_len]->key
                    //        );

                    v->arr_len++;


                } ZEND_HASH_FOREACH_END();
            }
            break;
        case IS_RESOURCE :
            /*
            v->type = "resource";
            resource_type = zend_rsrc_list_get_rsrc_type(Z_RES_P(expr) TSRMLS_CC);
            if (resource_type) {
                v->value = strdup(resource_type);
            }
            */
            break;
        case IS_NULL :
            v->type = "null";
            break;
    }


    return v;
}
/* }}} */

/* {{{ forp_find_symbol
 */
zval *forp_find_symbol(zend_string *name TSRMLS_DC) {
    HashTable *symbols = NULL;
    zval *val;
    int len = ZSTR_LEN(name);

    symbols = &EG(symbol_table);
    if (zend_hash_find(symbols, name)) {
        return zend_hash_find(symbols, name);
    }

    return NULL;
}
/* }}} */

/* {{{ forp_inspect_symbol
 *
void forp_inspect_symbol(zend_string *name TSRMLS_DC) {
    forp_var_t *v = NULL;
    zval **val;

    val = forp_find_symbol(name TSRMLS_CC);
    if(val != NULL) {
        forp_zval_var(v, *val, 1 TSRMLS_CC);
    } else {
        php_error_docref(
            NULL TSRMLS_CC,
            E_NOTICE,
            "Symbol not found : %s.",
            name
            );
    }
}
}}} */

/* {{{ forp_inspect_zval
 */
void forp_inspect_zval(char* name, zval *expr TSRMLS_DC) {
    forp_var_t *v = NULL;

    v = malloc(sizeof(forp_var_t));
    v->name = strdup(name);
    v->key = NULL;

    // if profiling started then attach the
    // current node index
    if(FORP_G(current_node)) {
        v->stack_idx = FORP_G(current_node)->key;
    } else {
        v->stack_idx = -1;
    }

    forp_zval_var(v, expr, 1 TSRMLS_CC);
    php_printf("%s\n", "brewk 1");
    return;

    FORP_G(inspect) = realloc(FORP_G(inspect), (FORP_G(inspect_len)+1) * sizeof(forp_var_t));
    FORP_G(inspect)[FORP_G(inspect_len)] = v;
    FORP_G(inspect_len)++;
}
/* }}} */