;; test-ac.l
;; Author: Yuki Furuta <furushchev@jsk.imi.i.u-tokyo.ac.jp>

(ros::load-ros-manifest "actionlib_tutorials")

(defparameter *init-pos* nil)

(defun ac-cb (server goal)
  (let ((res (send server :result)))
    (ros::ros-warn "callback")
    (setq *init-pos* 1)
    (ros::ros-warn "callback end")
    (return-from ac-cb
      (send server :set-succeeded res))))

(ros::roseus "ac_server")

(setq *server*
      (instance ros::simple-action-server :init
		"/fibonacci"
		actionlib_tutorials::FibonacciAction
		:execute-cb #'ac-cb)))
(ros::rate 10)
(while
    (ros::ok)
  (send *server* :worker)
  (ros::spin-once)
   (ros::sleep))
