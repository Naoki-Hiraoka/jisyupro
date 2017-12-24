(defun func (x) (print "ok"))
(defun func2 (x) (setq *a* x) (print "ok"))
(defun func3 (x) (print "ok") (setq *a* x))

(setq sharp-lambda #'(lambda (x) #|(setq *a* x)|# (print "ok")))
(setq quote-lambda '(lambda (x) #|(setq *a* x)|# (print "ok")))
(setq sharp-func #'func)
(setq quote-func 'func)
(setq sharp-lambda2 #'(lambda (x) (setq *a* x) (print "ok")))
(setq quote-lambda2 '(lambda (x) (setq *a* x) (print "ok")))
(setq sharp-func2 #'func2)
(setq quote-func2 'func2)
(setq sharp-lambda3 #'(lambda (x) (print "ok") (setq *a* x) ))
(setq quote-lambda3 '(lambda (x) (print "ok") (setq *a* x) ))
(setq sharp-func3 #'func3)
(setq quote-func3 'func3)

(defun temp-test1 (function)
  (let ((msg 1))
    (funcall function msg)))

(defun temp-test2 (function)
  (let ((msg 1))
    (setq *b* function)
    (funcall *b* 1)))


(defclass tempclass2
  :super ros::object
  :slots (function))
(defmethod tempclass2
  (:init (x)
	 (setq function x))
  (:cb (x)
       (print function)
       (funcall function x)))
