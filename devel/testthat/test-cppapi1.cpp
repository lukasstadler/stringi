// [[Rcpp::depends(stringi)]]
#include <stringi.h>
#include <stringi.cpp>
#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::export]]
CharacterVector test_dup(CharacterVector x, IntegerVector i) {
   return stri_dup(x, i);
}
