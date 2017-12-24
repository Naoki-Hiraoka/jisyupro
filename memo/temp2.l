#!/usr/bin/env roseus

(setq sharp-lambda #'(lambda (x) #|(setq *a* x)|# (print "ok")))
(setq quote-lambda '(lambda (x) #|(setq *a* x)|# (print "ok")))
(defun func (x) (setq *a* x) (print "ok"))
(setq sharp-func #'func)
(setq quote-func 'func)

(defun test1 ()
  (let ((msg 1))
    (funcall sharp-lambda msg)))

(defun test2 ()
  (funcall sharp-lambda 1))

;;undefined function sharp-lambda
(defun test3 () 
  (let ((msg 1))
    (funcall #'sharp-lambda msg)))

;;undefined function sharp-lambda
(defun test4 ()
  (funcall #'sharp-lambda 1))

(defun test5 ()
  (let ((msg 1))
    (funcall quote-lambda msg)))

(defun test6 ()
  (funcall quote-lambda 1))

;;undefined function quote-lambda
(defun test7 ()
  (let ((msg 1))
    (funcall #'quote-lambda msg)))

;;undefined function quote-lambda
(defun test8 ()
  (funcall #'quote-lambda 1))

(defun test9 ()
  (let ((msg 1))
    (funcall sharp-func msg)))

(defun test10 ()
  (funcall sharp-func 1))

;;undefined function sharp-func
(defun test11 ()
  (let ((msg 1))
    (funcall #'sharp-func msg)))

;;undefined function sharp-func
(defun test12 ()
  (funcall #'sharp-func 1))

(defun test13 ()
  (let ((msg 1))
    (funcall quote-func msg)))

(defun test14 ()
  (funcall quote-func 1))

;;undefined function sharp-func
(defun test15 ()
  (let ((msg 1))
    (funcall #'quote-func msg)))

(defun test16 ()
  (funcall #'quote-func 1))

(test1)
(test2)
;;(test3)
;;(test4)
(test5)
(test6)
;;(test7)
;;(test8)
(test9)
(test10)
;;(test11)
;;(test12)
(test13)
(test14)
;;(test15)
;;(test16)

;;(exit)
