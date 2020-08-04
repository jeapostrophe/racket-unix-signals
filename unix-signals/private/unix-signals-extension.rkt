#lang racket/base
(require racket/runtime-path
         ffi/unsafe
         ffi/unsafe/define
         ffi/unsafe/port)

(provide get-signal-fd
         get-signal-names
         set-signal-handler!
         lowlevel-send-signal!)

(define-runtime-path unix-signals "unix-signals.dylib")
(define-ffi-definer define-unix-signals (ffi-lib unix-signals))

(define-unix-signals setup_self_pipe
  (_fun -> _int))
(define-unix-signals lowlevel_send_signal
  (_fun _fixnum _int -> _bool))
(define-unix-signals prim_capture_signal
  (_fun _int _int -> _bool))
(define-unix-signals prim_get_signal_fd
  (_fun -> _int))

(unless (zero? (setup_self_pipe))
  (error 'setup_self_pipe))

(define lowlevel-send-signal!
  lowlevel_send_signal)

(define set-signal-handler!
  prim_capture_signal)

(define (get-signal-fd)
  (unsafe-file-descriptor->port
   (prim_get_signal_fd)
   'signal-fd
   '(read)))

(define-unix-signals prim_get_signal_names_count
  (_fun -> _int))
(define-unix-signals prim_get_signal_names_name
  (_fun _int -> _symbol))
(define-unix-signals prim_get_signal_names_num
  (_fun _int -> _int))

(define signal-names-ht (make-hasheq))
(for ([i (in-range (prim_get_signal_names_count))])
  (hash-set! signal-names-ht
             (prim_get_signal_names_name i)
             (prim_get_signal_names_num i)))

(define (get-signal-names)
  signal-names-ht)
