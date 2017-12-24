#!usr/bin/env roseus

(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "jisyupro")
(load "package://dxl_armed_turtlebot/euslisp/dxl-armed-turtlebot-interface.l")
(dxl-armed-turtlebot-init)

;;end-coordsを意味のある位置に
(send (send *dxl-armed-turtlebot* :arm :end-coords) :move-to (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :translate (float-vector 133 -27 -13.5))) :world)

;;cameraの座標を作成
(setq *realsense-camera* (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :transform (make-coords :pos #f(133 -33 -93.5) :rpy (float-vector 0 pi/2 pi)))))
(send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :assoc *realsense-camera*)

;;poleの座標を購読
(setq *pole-detector-time* (ros::time-now))
(setq *poles* nil)
(ros::subscribe "/pole_detector/output" jisyupro::Vector3Array
		#'(lambda (msg)
		    (let* ((z (- (elt (send *realsense-camera* :worldpos) 2) 170))
			   (poss (mapcar #'ros::tf-point->pos (send msg :vector3s)))
			   (scaledposs (mapcar #'(lambda (n) (scale z n)) poss)))
		      (setq *poles* (mapcar #'(lambda (n)
						(send (send (send *realsense-camera* :copy-worldcoords) :transform (make-coords :pos n)) :worldpos))
					    scaledposs))
		      (setq *pole-detector-time* (send msg :header :stamp))
		      )))

(defun get-ball-state ()
  (setq *c* (instance ros::simple-action-client :init "board_state_recorder" jisyupro::board_state_recorderAction))
  (setq goal (instance jisyupro::board_state_recorderActiongoal :init))
  (send goal :goal :start t)
  (send *c* :send-goal goal)
  (send *c* :wait-for-result)
  (send (send *c* :get-result) :result :int8s)
)

(defun z-inverse-kinematics (target)
  (let (temp-target)
    (dotimes (i 50)
      (print i)
      (setq temp-target (v+ target (float-vector 0.0 0.0 i))) 
      (setq result (send *dxl-armed-turtlebot* :inverse-kinematics (make-cascoords :pos temp-target :rpy (float-vector 0 0 pi)) :rotation-axis :z :rthre (deg2rad 10) :thre 1 :link-list (send *dxl-armed-turtlebot* :link-list (send *dxl-armed-turtlebot* :arm :end-coords :parent)) :revert-if-fail t))
      (when result
	(print (send *dxl-armed-turtlebot* :arm :end-coords))
	(return-from z-inverse-kinematics result))
      )
    nil
    )
  )
