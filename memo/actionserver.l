(ros::load-ros-manifest "hiraoka_semi")

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
		hiraoka_semi::background_subtractor2Action
		:execute-cb #'ac-cb)))
(ros::rate 10)
(while
    (ros::ok)
  (send *server* :worker)
  (ros::spin-once)
   (ros::sleep))
