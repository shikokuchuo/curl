#' Event-driven Async Requests
#'
#' Async family functions are an alternative to the multi family that run
#' asynchronously. \code{async_run} is a threaded function that performs all
#' operations to completion asynchronously in a non-blocking manner. Async
#' functions are thread-safe versions of multi functions, and are otherwise
#' identical.
#'
#' All async functions do not default to the global pool. This helps to ensure
#' that multi functions are not inadvertently called on pools used for async
#' operations. The use of async and multi functions should never be mixed as the
#' multi functions are not thread-safe.
#'
#' @param pool A multi handle created by \link{new_pool}. This must be supplied,
#' and there is no default.
#' @inheritParams multi
#'
#' @return For \code{async_run}: invisible NULL. For other functions, the return
#'   value of their multi equivalents.
#'
#' @examplesIf requireNamespace("later", quietly = TRUE) && requireNamespace("nanonext", quietly = TRUE)
#' success <- function(x) results <<- append(results, list(x))
#' failure <- function(str) cat("Failed request:", str, "\n", file = stderr())
#'
#' results <- list()
#' pool <- curl::new_pool()
#'
#' # Create handles
#' h1 <- new_handle(url = "https://hb.cran.dev/delay/2")
#' h2 <- new_handle(url = "https://hb.cran.dev/delay/1")
#'
#' # Add first handle:
#' async_add(h1, done = success, fail = failure, pool = pool)
#'
#' # Perform the requests
#' async_run(pool)
#'
#' # Add second handle during the async request:
#' Sys.sleep(1)
#' async_add(h2, done = success, fail = failure, pool = pool)
#' async_fdset(pool)
#'
#' # Wait for completion
#' Sys.sleep(3)
#' later::run_now()
#'
#' # check both results are present
#' length(results)
#'
#' @name async
#' @rdname async
#' @seealso Multi interface: [multi]
#' @useDynLib curl R_multi_async
#' @export
async_run <- function(pool) {

  stopifnot(inherits(pool, "curl_multi"))
  cv <- attr(pool, "cv")
  if (is.null(cv)) attr(pool, "cv") <- cv <- nanonext::cv()
  invisible(.Call(R_multi_async, pool, cv))

}

#' @rdname async
#' @export
async_add <- function(handle, done = NULL, fail = NULL, data = NULL, pool) {
  cv <- attr(pool, "cv")
  if (is.null(cv)) attr(pool, "cv") <- cv <- nanonext::cv()
  nanonext::with_lock(
    cv,
    multi_add(handle = handle, done = done, fail = fail, data = data, pool = pool)
  )
}

#' @rdname async
#' @export
async_fdset <- function(pool) {
  cv <- attr(pool, "cv")
  if (is.null(cv)) attr(pool, "cv") <- cv <- nanonext::cv()
  nanonext::with_lock(cv, multi_fdset(pool = pool))
}
