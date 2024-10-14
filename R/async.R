#' Event-driven Async Requests
#'
#' \code{multi_async} is a threaded function that performs all operations to
#' completion asynchronously in a non-blocking manner.
#'
#' This function does not default to the global pool. This helps to ensure that
#' other functions are not unintentionally called on it concurrently, as it is
#' not safe to do so.
#'
#' Once this function is called, further actions must not be made on the
#' supplied pool until all async operations have completed.
#'
#' @param pool A multi handle created by \link{new_pool}. This must be supplied,
#' and there is no default.
#'
#' @return An external pointer to the created thread.
#'
#' @examplesIf requireNamespace("later", quietly = TRUE)
#' results <- list()
#' success <- function(x){
#'   results <<- append(results, list(x))
#' }
#' failure <- function(str){
#'   cat(paste("Failed request:", str, "\n"), file = stderr())
#' }
#'
#' # Create custom pool - required for multi_async()
#' pool <- new_pool()
#'
#' # This handle will take longest (2sec)
#' h1 <- new_handle(url = "https://hb.cran.dev/delay/2")
#' multi_add(h1, done = success, fail = failure, pool = pool)
#'
#' # This handle raises an error
#' h2 <- new_handle(url = "https://urldoesnotexist.xyz")
#' multi_add(h2, done = success, fail = failure, pool = pool)
#'
#' # Actually perform the requests
#' multi_async(pool)
#'
#' # Check the results (after waiting for completion)
#' Sys.sleep(3)
#' later::run_now()
#' results
#'
#' @seealso Multi interface: [multi]
#' @useDynLib curl R_multi_async
#' @export
multi_async <- function(pool) {

  stopifnot(inherits(pool, "curl_multi"))
  invisible(.Call(R_multi_async, pool))

}
