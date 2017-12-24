#!/usr/bin/env roseus

(setq a #'(lambda (x) (setq *a* x)))

(defun func1()
  (let ((msg 1))
    (funcall a msg)))

(defun func2 ()
  (funcall a 1))

(defun func3 ()
  (let ((msg 1))
    (funcall #'(lambda (x) (setq *a* x)) msg)))
(defun func4 ()
  (funcall #'(lambda (x) (setq *a* x)) 1))

(func1)
(func2)
(func3)
(func4)
