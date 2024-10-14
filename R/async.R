#' Event-driven Async Requests
#'
#' These functions perform all operations to completion asynchronously in a
#' non-blocking manner using threading.
#'
#' @inheritParams multi_run
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
#' # This handle will take longest (2sec)
#' h1 <- new_handle(url = "https://hb.cran.dev/delay/2")
#' multi_add(h1, done = success, fail = failure)
#'
#' # This handle raises an error
#' h2 <- new_handle(url = "https://urldoesnotexist.xyz")
#' multi_add(h2, done = success, fail = failure)
#'
#' # Actually perform the requests
#' multi_async()
#'
#' # Check the results (after waiting for completion)
#' Sys.sleep(3)
#' later::run_now()
#' results
#'
#' @seealso Multi interface: [multi]
#' @useDynLib curl R_multi_async
#' @export
multi_async <- function(pool = NULL) {

  if(is.null(pool))
    pool <- multi_default()
  stopifnot(inherits(pool, "curl_multi"))
  invisible(.Call(R_multi_async, pool))

}
