#!/usr/bin/env roseus

(setq floor0 (integer-vector
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0))
(setq floor1 (integer-vector
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0))
(setq floor2 (integer-vector
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0))
(setq floor3 (integer-vector
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0
	      0 0 0 0))

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
;;ボールの状況の強制更新
(ros::advertise "/main/ball_state_renewal" jisyupro::Int8Array 1)
(setq msg (instance jisyupro::Int8Array :init))
(send msg :header :stamp (ros::time-now))
(send msg :int8s board_state)
(ros::rate 10)
(while (zerop (ros::get-num-subscribers "/main/ball_state_renewal"))
  (ros::sleep))
(ros::publish "/main/ball_state_renewal" msg)
