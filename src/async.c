#include <R.h>
#include <Rinternals.h>
#include <unistd.h>
#include "curl-common.h"

typedef struct multi_thread_arg_s {
  SEXP ptr;
  CURLM *multi;
} multi_thread_arg;

static void thread_arg_finalizer(SEXP xptr) {

  if (R_ExternalPtrAddr(xptr) == NULL) return;
  multi_thread_arg *xp = (multi_thread_arg *) R_ExternalPtrAddr(xptr);
  R_Free(xp);

}

static void printf_safe(const int err, const char *fmt, ...) {

  char buf[1024];
  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  int bytes = vsnprintf(buf, 1024, fmt, arg_ptr);
  va_end(arg_ptr);

  if (write(err ? STDERR_FILENO : STDOUT_FILENO, buf, (size_t) bytes)) {};

}

void later2(void (*fun)(void *), void *data) {
  eln2(fun, data, 0, 0);
}

SEXP curl_thread_create(void (*func)(void *), void *arg) {
  return thread_create(func, arg);
}

static void invoke_multi_run(void *arg) {
  SEXP func, call, pool = (SEXP) arg;
  Rf_classgets(pool, Rf_mkString("curl_multi"));
  PROTECT(func = Rf_lang3(R_DoubleColonSymbol, Rf_install("curl"), Rf_install("multi_run")));
  PROTECT(call = Rf_lang4(func, Rf_ScalarInteger(0), Rf_ScalarLogical(0), pool));
  Rf_eval(call, R_GlobalEnv);
  UNPROTECT(2);
  Rf_setAttrib(pool, R_MissingArg, R_NilValue);
  Rprintf("called from later: async ops complete\n");
}


static void multi_async_thread(void *arg) {

  multi_thread_arg *args = (multi_thread_arg *) arg;
  CURLM *multi = args->multi;

  int total_pending = -1;
  int numfds, count = 0;
  CURLMcode res;

  while (total_pending != 0) {

    res = CURLM_CALL_MULTI_PERFORM;
    while (res == CURLM_CALL_MULTI_PERFORM)
      res = curl_multi_perform(multi, &(total_pending));

    if (res != CURLM_OK) {
      printf_safe(1, "curl error: %s\n", curl_easy_strerror(res));
      break;
    }

    if ((res = curl_multi_wait(multi, NULL, 0, 10000, &numfds)) != CURLM_OK) {
      printf_safe(1, "curl error: %s\n", curl_easy_strerror(res));
      break;
    }
    count++;

  }

  printf_safe(0, "\nnumber of waits: %d\n", count);
  later2(invoke_multi_run, args->ptr);

}

SEXP R_multi_async(SEXP pool_ptr) {

  multiref *mref = get_multiref(pool_ptr);
  CURLM *multi = mref->m;
  multi_thread_arg *args = R_Calloc(1, multi_thread_arg);
  args->multi = multi;
  // TODO: add SEXP to an internal precious list for safety
  args->ptr = pool_ptr;
  SEXP out, xptr;

  if (thread_create == NULL) {
    SEXP str, call;
    PROTECT(str = Rf_mkString("nanonext"));
    PROTECT(call = Rf_lang2(Rf_install("loadNamespace"), str));
    Rf_eval(call, R_BaseEnv);
    UNPROTECT(2);
    thread_create = (SEXP (*)(void (*func)(void *), void *arg)) R_GetCCallable("nanonext", "rnng_thread_create");
  }

  if (eln2 == NULL) {
    SEXP str, call;
    PROTECT(str = Rf_mkString("later"));
    PROTECT(call = Rf_lang2(Rf_install("loadNamespace"), str));
    Rf_eval(call, R_BaseEnv);
    UNPROTECT(2);
    eln2 = (void (*)(void (*)(void *), void *, double, int)) R_GetCCallable("later", "execLaterNative2");
  }

  out = curl_thread_create(multi_async_thread, args);
  Rf_setAttrib(pool_ptr, R_MissingArg, out);
  xptr = R_MakeExternalPtr(args, R_NilValue, R_NilValue);
  Rf_setAttrib(out, R_MissingArg, xptr);
  R_RegisterCFinalizerEx(xptr, thread_arg_finalizer, TRUE);
  Rf_classgets(pool_ptr, R_NilValue);

  return out;

}
