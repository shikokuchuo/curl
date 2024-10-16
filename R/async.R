#' Event-driven Async Requests
#'
#' \code{multi_async} is a threaded function that performs all operations to
#' completion asynchronously in a non-blocking manner.
#'
#' This function does not default to the global pool and requires a custom pool
#' to be passed to it. This is as the pool is made temporarily unavailable to
#' other functions whilst async operations are ongoing. Attempting to use the
#' pool in such a case will result in the error
#' \code{inherits(pool, "curl_multi") is not TRUE}.
#'
#' @param pool A multi handle created by \link{new_pool}. This must be supplied,
#' and there is no default.
#'
#' @return An external pointer to the created thread.
#'
#' @examplesIf requireNamespace("later", quietly = TRUE)
#' success <- function(x){
#' results <<- append(results, list(x))
#' }
#'
#' failure <- function(str){
#' cat(paste("Failed request:", str, "\n"), file = stderr())
#' }
#'
#' results <- list()
#' pool <- curl::new_pool()
#'
#' # Create handles
#' h1 <- new_handle(url = "https://hb.cran.dev/delay/2")
#' h2 <- new_handle(url = "https://hb.cran.dev/delay/1")
#'
#' # Add handles:
#' multi_add(h1, done = success, fail = failure, pool = pool)
#' multi_add(h2, done = success, fail = failure, pool = pool)
#'
#' # Actually perform the requests
#' multi_async(pool = pool)
#'
#' # Wait for completion
#' Sys.sleep(3)
#' later::run_now()
#'
#' # check all results are present
#' length(results)
#'
#' @seealso Multi interface: [multi]
#' @useDynLib curl R_multi_async
#' @export
multi_async <- function(pool) {

  stopifnot(inherits(pool, "curl_multi"))
  invisible(.Call(R_multi_async, pool))

}
