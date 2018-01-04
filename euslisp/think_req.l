#!/usr/bin/env roseus

(setq floor0 (integer-vector
	      1 0 1 0
	      -1 1 1 0
	      0 0 0 0
	      -1 0 0 0))
(setq floor1 (integer-vector
	      0 0 0 0
	      -1 0 0 0
	      0 0 0 0
	      -1 0 0 0))
(setq floor2 (integer-vector
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0
	      -1 0 0 0))
(setq floor3 (integer-vector
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0
	      1 0 0 0))

(setq board_state (make-array 64 :element-type :integer))
(dotimes (i 16)
  (setf (elt board_state (+ (* i 4) 0))
	(elt floor0 i))
  (setf (elt board_state (+ (* i 4) 1))
	(elt floor1 i))
  (setf (elt board_state (+ (* i 4) 2))
	(elt floor2 i))
  (setf (elt board_state (+ (* i 4) 3))
	(elt floor3 i)))
(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "jisyupro")

(ros::roseus "renewal")

(setq *ct* (instance ros::simple-action-client :init "/think" jisyupro::thinkAction))
(defun think ()
  (ros::ros-info "think")
  (let ((goal (instance jisyupro::thinkActiongoal :init)))
    (send goal :goal :board :int8s board_state)
    (send goal :goal :board :header :stamp (ros::time-now))
    (send *ct* :send-goal goal)
    (send *ct* :wait-for-result)
    (setq *place-target* (send (send *ct* :get-result) :newpos)))
  )
