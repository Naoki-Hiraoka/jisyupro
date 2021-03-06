#!usr/bin/env roseus

(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "hiraoka_semi")
(load "package://dxl_armed_turtlebot/euslisp/dxl-armed-turtlebot-interface.l")
(dxl-armed-turtlebot-init)

;;end-coordsを意味のある位置に
(send (send *dxl-armed-turtlebot* :arm :end-coords) :move-to (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :translate (float-vector 133 -27 -13.5))) :world)

;;cameraの座標を作成
(setq *realsense-camera* (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :transform (make-coords :pos #f(133 -33 -93.5) :rpy (float-vector 0 pi/2 pi)))))

(send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :assoc *realsense-camera*)

;;手を上に上げる方法
;;send *dxl-armed-turtlebot* :reset-pose2
;;(send *dxl-armed-turtlebot* :inverse-kinematics (make-cascoords :pos #f(300 0 700) :rpy (float-vector 0 -pi/2 0)) :rotation-axis t :rthre (deg2rad 1) :thre 10 :link-list (send *dxl-armed-turtlebot* :link-list (send *dxl-armed-turtlebot* :arm :end-coords :parent)) :revert-if-fail nil)

;; (send *dxl-armed-turtlebot* :inverse-kinematics (make-cascoords :pos (float-vector 350 -50 140) :rpy (float-vector 0 pi/2 0)) :rotation-axis :x :rthre (deg2rad 10) :thre 1 :link-list (send *dxl-armed-turtlebot* :link-list (send *dxl-armed-turtlebot* :arm :end-coords :parent)) :revert-if-fail t :stop 100)



;;+-100mmはなかなか動けないと思ったほうがいい
;;z軸方向に目標位置を少しずつ変えながら複数回解けばどうか

;;本体に当たりそう
;;200 100 80

;;250は、y=0に行けないのが致命的
;;250 -100 90
;;250 -50 80
;;250 0 x
;;250 50 73
;;250 100 80

;;300,350は、上方へはある程度可動域がある。
;;このあたりの、z=120位をピンの位置にすると良いか
;;300 -100 115
;;300 -50 85
;;300 0 78 これは上方に無理
;;300 50 80
;;300 100 100
;;350 -100 100
;;350 -50 100
;;350 0 115
;;350 50 100
;;350 100 100
;;400 0 110


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

;;hiraoka_semi video_recorder
;;hiraoka_semi background_subtractor
(defun ouhuku-test ()
  ;;動画を撮る
  (setq file_name "/tmp/video.avi")
  (setq topic_name "/camera/rgb/image_rect_color")
  (setq req (instance hiraoka_semi::video_recorderrequest :init))
  (send req :filename file_name)
  (send req :topicname topic_name)
  (setq req2 (copy-object req))
  (send req :start t)
  (ros::ros-info "video start")
  (ros::wait-for-service "/video_recorder")
  (ros::service-call "/video_recorder" req)
  (unix::sleep 20)
  (send req2 :stop t)
  (ros::ros-info "video stop")
  (ros::service-call "/video_recorder" req2)
  ;;32往復
  (dotimes (i 10)
    (send *ri* :go-pos 0.2 0 0)
    (send *ri* :wait-interpolation)
    (unix::usleep 100000)
    (send *ri* :go-pos -0.2 0 0)
    (send *ri* :wait-interpolation)
    (unix::usleep 100000)
    )
  ;;差分を取らせる
  (setq req (instance dynamic_reconfigure::Reconfigurerequest :init))
  (send req :config :strs (list (instance dynamic_reconfigure::StrParameter :init :name "filename" :value file_name) (instance dynamic_reconfigure::StrParameter :init :name "topicname" :value topic_name)))   
  (ros::wait-for-service "/background_subtractor/set_parameters")
  (ros::service-call "/background_subtractor/set_parameters" req)
  )

;;end-coordsの位置を変える必要がある。
;;そもそも持てるの?
