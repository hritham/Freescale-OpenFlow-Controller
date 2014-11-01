/*
 * Summary: macros for marking symbols as exportable/importable.
 * Description: macros for marking symbols as exportable/importable.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Igor Zlatovic <igor@zlatkovic.com>
 */

#ifdef OF_XML_SUPPORT
/*#ifdef OF_XML_SCHEMA_VAL_SUPPORT*/

#ifndef __XML_EXPORTS_H__
#define __XML_EXPORTS_H__

/**
 * XMLPUBFUN, XMLPUBVAR, XMLCALL
 *
 * Macros which declare an exportable function, an exportable variable and
 * the calling convention used for functions.
 *
 * Please use an extra block for every platform/compiler combination when
 * modifying this, rather than overlong #ifdef lines. This helps
 * readability as well as the fact that different compilers on the same
 * platform might need different definitions.
 */

/**
 * XMLPUBFUN:
 *
 * Macros which declare an exportable function
 */
#define XMLPUBFUN
/**
 * XMLPUBVAR:
 *
 * Macros which declare an exportable variable
 */
#define XMLPUBVAR extern
/**
 * XMLCALL:
 *
 * Macros which declare the called convention for exported functions
 */
#define XMLCALL

#endif /* __XML_EXPORTS_H__ */

/*#endif*/ /* OF_XML_SCHEMA_VAL_SUPPORT */
#endif /* OF_XML_SUPPORT */
