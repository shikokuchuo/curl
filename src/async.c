#ifdef _WIN32
#include <winsock2.h>
#endif
#include <R.h>
#include <Rinternals.h>
#include <unistd.h>
#include "curl-common.h"

typedef struct multi_thread_arg_s {
  SEXP ptr;
  SEXP cv;
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
  int maxfd = -1;
  int count = 0;
  long timeo;
  struct timeval timeout;
  CURLMcode res;
  fd_set fdread, fdwrite, fdexc;

  res = CURLM_CALL_MULTI_PERFORM;
  cv_lock(args->cv);
  while (res == CURLM_CALL_MULTI_PERFORM)
    res = curl_multi_perform(multi, &(total_pending));
  cv_unlock(args->cv);

  if (res != CURLM_OK) {
    printf_safe(1, "curl error: %s\n", curl_easy_strerror(res));
  }

  while (total_pending != 0) {

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexc);

    // Get file descriptors for active sockets
    res = curl_multi_fdset(multi, &fdread, &fdwrite, &fdexc, &maxfd);

    curl_multi_timeout(multi, &timeo);
    if (timeo == -1) {
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
    } else {
      timeout.tv_sec = timeo / 1000;
      timeout.tv_usec = (timeo % 1000) * 1000;
    }

    if (maxfd >= 0) {
      if (select(maxfd + 1, &fdread, &fdwrite, &fdexc, &timeout) < 0)
        printf_safe(1, "select error\n");
    }
    count++;

    res = CURLM_CALL_MULTI_PERFORM;
    cv_lock(args->cv);
    while (res == CURLM_CALL_MULTI_PERFORM)
      res = curl_multi_perform(multi, &(total_pending));
    cv_unlock(args->cv);

    if (res != CURLM_OK) {
      printf_safe(1, "curl error: %s\n", curl_easy_strerror(res));
      break;
    }

  }

  printf_safe(0, "\nnumber of waits: %d\n", count);
  later2(invoke_multi_run, args->ptr);

}

SEXP R_multi_async(SEXP pool_ptr, SEXP cv) {

  if (thread_create == NULL) {
    SEXP str, call;
    PROTECT(str = Rf_mkString("nanonext"));
    PROTECT(call = Rf_lang2(Rf_install("loadNamespace"), str));
    Rf_eval(call, R_BaseEnv);
    UNPROTECT(2);
    thread_create = (SEXP (*)(void (*func)(void *), void *arg)) R_GetCCallable("nanonext", "rnng_thread_create");
    cv_lock = (SEXP (*)(SEXP)) R_GetCCallable("nanonext", "rnng_cv_lock");
    cv_unlock = (SEXP (*)(SEXP)) R_GetCCallable("nanonext", "rnng_cv_unlock");
  }

  if (eln2 == NULL) {
    SEXP str, call;
    PROTECT(str = Rf_mkString("later"));
    PROTECT(call = Rf_lang2(Rf_install("loadNamespace"), str));
    Rf_eval(call, R_BaseEnv);
    UNPROTECT(2);
    eln2 = (void (*)(void (*)(void *), void *, double, int)) R_GetCCallable("later", "execLaterNative2");
  }

  if (TYPEOF(cv) != EXTPTRSXP || EXTPTR_TAG(cv) != Rf_install("cv"))
    Rf_error("'cv' is not a valid Condition Variable");

  multiref *mref = get_multiref(pool_ptr);
  CURLM *multi = mref->m;
  multi_thread_arg *args = R_Calloc(1, multi_thread_arg);
  args->multi = multi;
  // TODO: consider preserving SEXPs
  args->ptr = pool_ptr;
  args->cv = cv;
  SEXP out, xptr;

  PROTECT(out = curl_thread_create(multi_async_thread, args));
  xptr = R_MakeExternalPtr(args, R_NilValue, R_NilValue);
  Rf_setAttrib(out, R_MissingArg, xptr);
  R_RegisterCFinalizerEx(xptr, thread_arg_finalizer, TRUE);
  Rf_setAttrib(pool_ptr, R_MissingArg, out);

  UNPROTECT(1);
  return R_NilValue;

}
