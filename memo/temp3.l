#!/usr/bin/env roseus

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

(defun test1 (function)
  (let ((msg 1))
    (funcall function msg)))

(defun test2 (function)
  (funcall function 1))

;;undefined function sharp-lambda
(defun test3 (funciton) 
  (let ((msg 1))
    (funcall #'function msg)))

;;undefined function sharp-lambda
(defun test4 ()
  (funcall #'function 1))


(test1 #'(lambda (x) #|(setq *a* x)|# (print "ok")))
(test1 '(lambda (x) #|(setq *a* x)|# (print "ok")))
(test1 #'func)
(test1 'func)
;;(test1 func) unbound variable func

(test2 #'(lambda (x) #|(setq *a* x)|# (print "ok")))
(test2 '(lambda (x) #|(setq *a* x)|# (print "ok")))
(test2 #'func)
(test2 'func)
;;(test2 func) unbound variable func

;;(test3 #'(lambda (x) #|(setq *a* x)|# (print "ok")))  illegal function test3 in (funcall #'function msg)
;;(test3 '(lambda (x) #|(setq *a* x)|# (print "ok"))) illegal function test3 in (funcall #'function msg)
;;(test3 #'func) illegal function test3 in (funcall #'function msg)
;;(test3 'func) illegal function test3 in (funcall #'function msg)
;;(test3 func) unbound variable func

;;(test4 #'(lambda (x) #|(setq *a* x)|# (print "ok"))) illegal function test3 in (funcall #'function msg)
;;(test4 '(lambda (x) #|(setq *a* x)|# (print "ok"))) illegal function test3 in (funcall #'function msg)
;;(test4 #'func) illegal function test3 in (funcall #'function msg)
;;(test4 'func) illegal function test3 in (funcall #'function msg)
;;(test4 func) unbound variable func

(test1 #'(lambda (x) (setq *a* x) (print "ok")))
(test1 '(lambda (x) (setq *a* x) (print "ok")))
(test1 #'func2)
(test1 'func2)

(test2 #'(lambda (x) (setq *a* x) (print "ok")))
(test2 '(lambda (x) (setq *a* x) (print "ok")))
(test2 #'func2)
(test2 'func2)

(test1 sharp-lambda)
(test1 quote-lambda)
(test1 sharp-func)
(test1 quote-func)

(test2 sharp-lambda);;手動-> Segmentation Fault. (非emacsでも起こる)
(test2 quote-lambda)
(test2 sharp-func)
(test2 quote-func)

;;(test3 sharp-lambda) illegal function test3 in (funcall #'function msg)
;;(test3 quote-lambda)
;(test3 sharp-func)
;;(test3 quote-func)

;;(test4 sharp-lambda)
;;(test4 quote-lambda)
;;(test4 sharp-func)
;;(test4 quote-func)

(test1 sharp-lambda2);;手動->never return　あい(非emacsでも起こる)
(test1 quote-lambda2)
(test1 sharp-func2);;手動->Segmentation Fault.　(非emacsではnever return)
(test1 quote-func2)

(test2 sharp-lambda2);;手動->Segmentation Fault.　(非emacsでもおこる)
(test2 quote-lambda2)
(test2 sharp-func2);;手動->Segmentation Fault.　(非emacsでもおこる)
(test2 quote-func2)

(test1 #'(lambda (x) (print "ok") (setq *a* x)))
(test1 '(lambda (x) (print "ok") (setq *a* x)))
(test1 #'func3)
(test1 'func3)
(test2 #'(lambda (x) (print "ok") (setq *a* x)))
(test2 '(lambda (x) (print "ok") (setq *a* x)))
(test2 #'func3)
(test2 'func3)

(let ((msg 1)) (funcall '(lambda (x) (setq *a* x) (print "ok")) msg))
(funcall '(lambda (x) (setq *a* x) (print "ok")) 1)
(let ((msg 1)) (funcall #'(lambda (x) (setq *a* x) (print "ok")) msg))
(funcall #'(lambda (x) (setq *a* x) (print "ok")) 1)

(let ((msg 1)) (funcall 'func3 msg))
(funcall 'func3 1)
(let ((msg 1)) (funcall #'func3 msg))
(funcall #'func3 1)

(let ((msg 1)) (funcall sharp-lambda msg))
(funcall sharp-lambda 1);;手動->Segmentation Fault (非emacsでも起こる)
;;(let ((msg 1)) (funcall #'sharp-lambda msg)) undefined function sharp-lambda
;;(funcall #'sharp-lambda 1)

(let ((msg 1)) (funcall quote-lambda msg))
(funcall quote-lambda 1)
;;(let ((msg 1)) (funcall #'quote-lambda msg)) undefined function quote-lambda
;;(funcall #'quote-lambda 1)


(let ((msg 1)) (funcall sharp-lambda2 msg));;手動->Segmentation Fault　い　(非emacsでもおこる)
(funcall sharp-lambda2 1);;手動->Segmentation Fault 　(非emacsでもおこる)
(let ((msg 1)) (funcall quote-lambda2 msg))
(funcall quote-lambda2 1)

(let ((msg 1)) (funcall sharp-func2 msg));;手動->Segmentation Fault 　(非emacsでもおこる)
(funcall sharp-func2 1);;手動->Segmentation Fault 　(非emacsでもおこる)
(let ((msg 1)) (funcall quote-func2 msg))
(funcall quote-func2 1)

(let ((msg 1)) (funcall sharp-lambda3 msg));;手動->Segmentation Fault い　(非emacsでもおこる)
(funcall sharp-lambda3 1);;手動->Segmentation Fault 　(非emacsでもおこる)
(let ((msg 1)) (funcall quote-lambda3 msg))
(funcall quote-lambda3 1)

(let ((msg 1)) (funcall sharp-func3 msg));;手動->Segmentation Fault 　(非emacsでもおこる)
(funcall sharp-func3 1);;手動->Segmentation Fault 　(非emacsでもおこる)
(let ((msg 1)) (funcall quote-func3 msg))
(funcall quote-func3 1)

;;実行後、手動で行う
(print sharp-lambda)
(print quote-lambda)
(print sharp-func)
(print quote-func)
(print sharp-lambda2)
(print quote-lambda2)
(print sharp-func2)
(print quote-func2)
(print sharp-lambda3)
(print quote-lambda3)
(print sharp-func3)
(print quote-func3)

(let ((a 1))
  (test2 sharp-lambda))

(defun hoge ()
    (test2 sharp-lambda))
(hoge)
  ;;(exit)
