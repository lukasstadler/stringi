/* This file is part of the 'stringi' library.
 *
 * Copyright 2013 Marek Gagolewski, Bartek Tartanus, Marcin Bujarski
 *
 * 'stringi' is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * 'stringi' is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with 'stringi'. If not, see <http://www.gnu.org/licenses/>.
 */

#include "stringi.h"



/**
 * Count the number of recurrences of \code{pattern} in \code{str} [fast but dummy bitewise compare]
 *
 * @param str strings to search in
 * @param pattern patterns to search for
 * @return integer vector
 *
 * @version 0.1 (Bartek Tartanus)
 * @version 0.2 (Marek Gagolewski) - use StriContainerUTF8
 * @version 0.3 (Marek Gagolewski) - corrected behavior on empty str/pattern
 * @version 0.4 (Marek Gagolewski, 2013-06-23) make StriException-friendly,
 *    use StriContainerByteSearch
 */
SEXP stri__count_fixed_byte(SEXP str, SEXP pattern)
{
   str = stri_prepare_arg_string(str, "str");
   pattern = stri_prepare_arg_string(pattern, "pattern");

   STRI__ERROR_HANDLER_BEGIN
   int vectorize_length = stri__recycling_rule(true, 2, LENGTH(str), LENGTH(pattern));
   StriContainerUTF8 str_cont(str, vectorize_length);
   StriContainerByteSearch pattern_cont(pattern, vectorize_length);

   SEXP ret;
   PROTECT(ret = Rf_allocVector(INTSXP, vectorize_length));
   int* ret_tab = INTEGER(ret);

   for (R_len_t i = pattern_cont.vectorize_init();
      i != pattern_cont.vectorize_end();
      i = pattern_cont.vectorize_next(i))
   {
      STRI__CONTINUE_ON_EMPTY_OR_NA_STR_PATTERN(str_cont, pattern_cont,
      ret_tab[i] = NA_INTEGER, ret_tab[i] = 0)

      pattern_cont.setupMatcher(i, str_cont.get(i).c_str(), str_cont.get(i).length());
      ret_tab[i] = 0;
      while (USEARCH_DONE != pattern_cont.findNext())
         ++ret_tab[i];
   }

   UNPROTECT(1);
   return ret;
   STRI__ERROR_HANDLER_END( ;/* do nothing special on error */ )
}




/**
 * Count pattern occurcess in a string [with collation]
 *
 * @param str character vector
 * @param pattern character vector
 * @param collator_opts passed to stri__ucol_open(),
 * if \code{NA}, then \code{stri_detect_fixed_byte} is called
 * @return integer vector
 *
 * @version 0.1 (Marek Gagolewski)
 * @version 0.2 (Marek Gagolewski) - corrected behavior on empty str/pattern
 * @version 0.3 (Marek Gagolewski, 2013-06-23) make StriException-friendly,
 *    use StriContainerUStringSearch
 */
SEXP stri_count_fixed(SEXP str, SEXP pattern, SEXP collator_opts)
{
   str = stri_prepare_arg_string(str, "str");
   pattern = stri_prepare_arg_string(pattern, "pattern");

   // call stri__ucol_open after prepare_arg:
   // if prepare_arg had failed, we would have a mem leak
   UCollator* collator = stri__ucol_open(collator_opts);
   if (!collator)
      return stri__count_fixed_byte(str, pattern);

   STRI__ERROR_HANDLER_BEGIN
   R_len_t vectorize_length = stri__recycling_rule(true, 2, LENGTH(str), LENGTH(pattern));
   StriContainerUTF16 str_cont(str, vectorize_length);
   StriContainerUStringSearch pattern_cont(pattern, vectorize_length, collator);  // collator is not owned by pattern_cont

   SEXP ret;
   PROTECT(ret = Rf_allocVector(INTSXP, vectorize_length));
   int* ret_tab = INTEGER(ret);

   for (R_len_t i = pattern_cont.vectorize_init();
         i != pattern_cont.vectorize_end();
         i = pattern_cont.vectorize_next(i))
   {
      STRI__CONTINUE_ON_EMPTY_OR_NA_STR_PATTERN(str_cont, pattern_cont,
         ret_tab[i] = NA_INTEGER,
         ret_tab[i] = 0)

      UStringSearch *matcher = pattern_cont.getMatcher(i, str_cont.get(i));
      usearch_reset(matcher);
      UErrorCode status = U_ZERO_ERROR;
      ret_tab[i] = 0;
      while (((int)usearch_next(matcher, &status) != USEARCH_DONE) && !U_FAILURE(status))
         ++ret_tab[i];
      if (U_FAILURE(status)) throw StriException(status);
   }

   if (collator) { ucol_close(collator); collator=NULL; }
   UNPROTECT(1);
   return ret;
   STRI__ERROR_HANDLER_END(
      if (collator) ucol_close(collator);
   )
}
