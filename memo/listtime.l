(defun a () '(1))
(eval '(+ 3 4 5 ,@a))
