#' Event-driven Async Requests
#'
#' Async family functions modify the multi family to run asynchronously.
#'
#' \code{async_run} is a threaded function that performs all operations to
#' completion asynchronously in a non-blocking manner.
#'
#' This function does not default to the global pool. This helps to ensure that
#' other functions are not unintentionally called on it concurrently, as it is
#' not safe to do so.
#'
#' \code{async_add} is equivalent to \link{multi_add} but safe to use whilst an
#' async operation is ongoing.
#'
#' @param pool A multi handle created by \link{new_pool}. This must be supplied,
#' and there is no default.
#' @inheritParams multi
#'
#' @return For async_run: invisibly, an external pointer to the created thread.
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
#' thread <- async_run(pool = pool)
#'
#' # Add second handle, acquiring lock for safety:
#' Sys.sleep(1)
#' async_add(h2, done = success, fail = failure, pool = pool)
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
  if (is.null(cv)) {
    attr(pool, "cv") <- cv <- nanonext::cv()
  }
  invisible(.Call(R_multi_async, pool, cv))

}

#' @rdname async
#' @export
async_add <- function(handle, done = NULL, fail = NULL, data = NULL, pool) {
  cv <- attr(pool, "cv")
  if (is.null(cv)) {
    attr(pool, "cv") <- cv <- nanonext::cv()
  }
  nanonext::with_lock(
    cv,
    multi_add(handle = handle, done = done, fail = fail, data = data, pool = pool)
  )
}
