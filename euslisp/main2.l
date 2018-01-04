#!/usr/bin/env roseus

(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "jisyupro")
(ros::load-ros-manifest "std_msgs")
(when (require "package://dxl_armed_turtlebot/euslisp/dxl-armed-turtlebot-interface.l")
  (dxl-armed-turtlebot-init))

;;*****************************************************************************:
;;board_detector.l

;;カメラ支持部を作成
(let ((b1 (make-cube 50.0 10.0 410.0));;zを10大きくして食い込ませる
      (b2 (make-cube 50.0 10.0 410.0));;zを10大きくして食い込ませる
      (b3 (make-cube 558.0 50.0 10.0)));;x,yを2大きくして食い込ませる
  (send b1 :translate (float-vector 19.0 -19 610))
  (send b2 :translate (float-vector 19.0 19 610))
  (send b3 :translate (float-vector 272.0 0 815))
  (send *irtviewer* :draw-objects)
  (setq *supporter* (body+ b1 b3 b2))
  (send *supporter* :set-color :gray10)
  (send *supporter* :put :attach-coords (make-cascoords :pos (float-vector 550 0 810)))
  (send *supporter* :assoc (send *supporter* :get :attach-coords))
  (send *dxl-armed-turtlebot* :assoc *supporter*)
  (objects (list *dxl-armed-turtlebot* *supporter*)) 
  (send *irtviewer* :draw-objects))

;;cameraの座標を作成
;;始め手先につけようと思ったが、たわんでダメだと知った。
;;(setq *realsense-camera* (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :transform (make-coords :pos #f(133 -33 -93.5) :rpy (float-vector 0 pi/2 pi)))))
;;(send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :assoc *realsense-camera*)
;;(setq *realsense-camera* (make-cascoords :coords (send (send (send *supporter* :get :attach-coords) :copy-worldcoords) :rotate pi :x)))
(setq *realsense-camera* (make-cascoords :coords (send (send (send *supporter* :get :attach-coords) :copy-worldcoords) :transform (make-coords :pos #F(0 0 -5) :rpy (float-vector 0 0 pi)))))

;;poleの座標を購読
(setq *pole-list* nil)
(dotimes (i 16)
  (push (make-cylinder 5 150) *pole-list*))

(setq *draw-target* (append (list *dxl-armed-turtlebot* *supporter*) *pole-list*))
(objects *draw-target*)

(setq *board-detector-time* (ros::time-now))
(setq *pole-update* t)
(setq *a* nil)
;; (ros::subscribe "/board_detector/vector" jisyupro::Vector3Array
;; 		#'(lambda (msg)
;; 		    (when *pole-update*
;; 		      (setq *board-detector-time* (send msg :header :stamp))
;; 		      (dotimes (i 16)
;; 			(let ((cam->obj-vector (ros::tf-point->pos (elt (send msg :vector3s) i))))
;; 			  (send (elt *pole-list* i) :locate (send *realsense-camera* :transform-vector cam->obj-vector) :world)))
;; 		      (send *irtviewer* :draw-objects))))

;; (defun board_detector-cb (msg) ;;lambdaでやると、対話型インタプリタでros::spin-onceした時にnever returnする
;;   (when *pole-update*
;;     (setq *board-detector-time* (send msg :header :stamp))
;;     (dotimes (i 16)
;;       (let ((cam->obj-vector (ros::tf-point->pos (elt (send msg :vector3s) i))))
;; 	(send (elt *pole-list* i) :locate (send *realsense-camera* :transform-vector cam->obj-vector) :world)))
;;     (send *irtviewer* :draw-objects)))

(setq *my-color* 1);;red

(setq *board_detector-cb-average-num* -10)
(setq *board_detector-cb-average-msgs* nil)
(dotimes (i 10)
  (push nil *board_detector-cb-average-msgs*))
(defun board_detector-cb-average (msg) ;;過去10コマの平均値をとる ;;lambdaでやると、対話型インタプリタでros::spin-onceした時にnever returnする
  (when *pole-update*
    (send msg :vector3s (mapcar #'ros::tf-point->pos (send msg :vector3s)))
    (when (< *board_detector-cb-average-num* 0) ;;10コマ集まっていないとき
      (setf (elt *board_detector-cb-average-msgs* (+ *board_detector-cb-average-num* 10)) msg)
      (incf *board_detector-cb-average-num*)
      (return-from board_detector-cb-average))
    (setf (elt *board_detector-cb-average-msgs* *board_detector-cb-average-num*) msg)
    (incf *board_detector-cb-average-num*)
    (if (> *board_detector-cb-average-num* 9)
	(setq *board_detector-cb-average-num* 0))
    (setq *board-detector-time* (send (elt *board_detector-cb-average-msgs* *board_detector-cb-average-num*) :header :stamp));;計算に影響する最も古いtime
    ;;平均をとる
    (let ((sum (make-array 16 :initial-element #f(0.0 0.0 0.0))))
      (dotimes (i 16)
	(dotimes (j 10)
	  (setf (elt sum i) (v+ (elt sum i) (elt (send (elt *board_detector-cb-average-msgs* j) :vector3s) i)))
	  )
	(setf (elt sum i) (scale 0.1 (elt sum i)))
	;;poleの座標を変更
	(send (elt *pole-list* i) :locate (send *realsense-camera* :transform-vector (elt sum i)) :world)
	)
      )
    (send *irtviewer* :draw-objects)
    )
  ) 
(ros::subscribe "/board_detector/vector" jisyupro::Vector3Array #'board_detector-cb-average)

(defun wait-board-detector-time (&optional time) ;;画像処理が追いつくまで待つ関数
  (let ((ref-time (or time (ros::time-now))))
    (while (ros::time< *board-detector-time* ref-time)
      (ros::spin-once)
      (x::window-main-one)
      (ros::sleep))))

;;ボールの状況の更新
;;rostopic pub -1 /board_state_update std_msgs/Bool True
;;(setq *c* (instance ros::simple-action-client :init "board_state_recorder" jisyupro::board_state_recorderAction))
(setq *c* (instantiate ros::simple-action-client))
(send *c* :init "board_state_recorder" jisyupro::board_state_recorderAction)
(setq *ball-list* nil)
(setq *ball-vector* (make-array 64 :element-type :integer :initial-element 0))
(setq *pre-ball-vector* (make-array 64 :element-type :integer :initial-element 0))
;;カメラからボールの状況の更新
(defun update-ball-state ()
  ;; (let (goal)
  ;;   (setq goal (instance jisyupro::board_state_recorderActiongoal :init))
  ;;   (send goal :goal :update t)
  ;;   (send *c* :send-goal goal)
  ;;   (send *c* :wait-for-result)
  ;;   (setq *pre-ball-vector* *ball-vector*) 
  ;;   (setq *ball-vector* (send (send *c* :get-result) :board_state :int8s))
  ;;   )
  (setq goal (instance jisyupro::board_state_recorderActiongoal :init))
  (send goal :goal :update t)
  (send *c* :send-goal goal)
  (print "wait-for-reslt")
  (send *c* :wait-for-result)
  (print "get-reslt")
  (setq *pre-ball-vector* *ball-vector*) 
  (setq *ball-vector* (send (send *c* :get-result) :board_state :int8s))
  )
;;ball-vectorとpre-ball-vectorを比較し、変わった場所を返す。
;;不自然ならnilを返す
(setq *changepos* -1)
(defun check-ball-state ()
  (let ((change-pos nil))
    (dotimes (i 16)
      (let ((supported t))
	(dotimes (z 4)
	  ;;変化したか
	  (unless (= (elt *ball-vector* (+ (* i 4) z)) (elt *pre-ball-vector* (+ (* i 4) z)))
	    (if (zerop (elt *pre-ball-vector* (+ (* i 4) z))) 
		(if (and (null change-pos) supported)
		    (setq change-pos (+ (* i 4) z)) ;;change-posである
		  (return-from check-ball-state nil)) ;;change-posが複数ある/宙に浮いている
	      (return-from check-ball-state nil)) ;;食い込んでいる
	    )
	  (when (zerop (elt *ball-vector* (+ (* i 4) z)))
	    (setq supported nil))
	  )
	)
      )
    (setq *changepos* change-pos)
    )
  )
;;一つ前の認識に戻る
(defun ball-state-undo ()
  (setq goal (instance jisyupro::board_state_recorderActiongoal :init))
  (send goal :goal :undo t)
  (send *c* :send-goal goal)
  (send *c* :wait-for-result)
  (setq *pre-ball-vector* *ball-vector*) 
  (setq *ball-vector* (send (send *c* :get-result) :board_state :int8s))
  )
;;場所を教示
(defun ball-state-teach (newpos color)
  (setq goal (instance jisyupro::board_state_recorderActiongoal :init))
  (send goal :goal :teach t)
  (send goal :goal :newpos newpos)
  (send goal :goal :color color)
  (send *c* :send-goal goal)
  (send *c* :wait-for-result)
  (setq *pre-ball-vector* *ball-vector*) 
  (setq *ball-vector* (send (send *c* :get-result) :board_state :int8s))
  )
(setq *ball-state-teach* nil)
(setq *ball-state-teach-received* nil)
(defun ball-state-teach-cb (msg)
  (when *ball-state-teach*
    (ball-state-teach (send msg :data) (- *my-color*))
    (setq *ball-state-teach-received* t)))
(ros::subscribe "/main/ball_state_teach" std_msgs::Int32 #'ball-state-teach-cb)
;;内部状態の完全置き換え
(defun ball-state-renewal (new_state);;jisyupro/Int8Array
  (setq goal (instance jisyupro::board_state_recorderActiongoal :init))
  (when (null (send new_state :header :stamp))
    (send new_state :header :stamp (ros::time-now)))
  (send goal :goal :new_state new_state)
  (send *c* :send-goal goal)
  (send *c* :wait-for-result)
  (setq *pre-ball-vector* *ball-vector*) 
  (setq *ball-vector* (send (send *c* :get-result) :board_state :int8s))
  )
(setq *ball-state-renewal* nil)
(setq *ball-state-renewal-received* nil)
(defun ball-state-renewal-cb (msg)
  (when *ball-state-renewal*
    (ball-state-renewal (send msg :data) (- *my-color*))
    (setq *ball-state-teach-received* t)))
(ros::subscribe "/main/ball_state_renewal" jisyupro::Int8Array #'ball-state-renewal-cb)
;;ボールの表示
(defun draw-balls ()
  (dolist (ball *ball-list*)
    (send (send ball :parent) :dissoc ball))
  (setq *ball-list* nil)
  (dotimes (i 16)
    (dotimes (z 4)
      (when (= (elt *ball-vector* (+ (* i 4) z)) 1)
	(let ((ball (make-cube 64 64 30)))
	  (send ball :set-color :red)
	  (send ball :move-to (send (send (elt *pole-list* i) :copy-worldcoords) :locate (float-vector 0 0 (+ (* z 30) 15))))
	  (send (elt *pole-list* i) :assoc ball)
	  (push ball *ball-list*)
	  ))
      (when (= (elt *ball-vector* (+ (* i 4) z)) -1)
	(let ((ball (make-cube 64 64 30)))
	  (send ball :set-color :blue)
	  (send ball :move-to (send (send (elt *pole-list* i) :copy-worldcoords) :locate (float-vector 0 0 (+ (* z 30) 15))))
	  (send (elt *pole-list* i) :assoc ball)
	  (push ball *ball-list*)
	  ))
      ))
  (objects (append *draw-target* *ball-list*))
  t
  )

;;(ros::subscribe "/board_state_update" std_msgs::bool
;;		#'(lambda (msg)
;;		    (get-ball-state)))

;;初期姿勢をとる(カメラが有効に使える姿勢)
(defun default-pose ()
  ;;(send *dxl-armed-turtlebot* :reset-pose2)
  ;;(send *dxl-armed-turtlebot* :inverse-kinematics (make-cascoords :pos #f(300 0 700) :rpy (float-vector 0 -pi/2 0)) :rotation-axis t :rthre (deg2rad 1) :thre 10 :link-list (send *dxl-armed-turtlebot* :link-list (send *dxl-armed-turtlebot* :arm :end-coords :parent)) :revert-if-fail nil)

  ;;(send *dxl-armed-turtlebot* :tuckarm-pose2)
  (send *dxl-armed-turtlebot* :angle-vector #f(90.0 90.0 5.0 0.0 0.0 -180.0 0.0))
  (send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 3000)
  (send *ri* :wait-interpolation)
)

;;勝利判定
(defun victory-check ()
  (dotimes (i 16);;上下
    (when (= (elt *ball-vector* (+ (* i 4) 0))
	     (elt *ball-vector* (+ (* i 4) 1))
	     (elt *ball-vector* (+ (* i 4) 2))
	     (elt *ball-vector* (+ (* i 4) 3)))
      (unless (zerop (elt *ball-vector* (+ (* i 4) 0)))
	(return-from victory-check (elt *ball-vector* (+ (* i 4) 0))))))
  (dotimes (i 4);;横
    (dotimes (j 4)
      (when (= (elt *ball-vector* (+ (* i 16) (* 0 4) j))
	       (elt *ball-vector* (+ (* i 16) (* 1 4) j))
	       (elt *ball-vector* (+ (* i 16) (* 2 4) j))
	       (elt *ball-vector* (+ (* i 16) (* 3 4) j)))
	(unless (zerop (elt *ball-vector* (+ (* i 16) (* 0 4) j)))
	  (return-from victory-check (elt *ball-vector* (+ (* i 16) (* 0 4) j)))))))
  (dotimes (i 4);;縦
    (dotimes (j 4)
      (when (= (elt *ball-vector* (+ (* 0 16) (* i 4) j))
	       (elt *ball-vector* (+ (* 1 16) (* i 4) j))
	       (elt *ball-vector* (+ (* 2 16) (* i 4) j))
	       (elt *ball-vector* (+ (* 3 16) (* i 4) j)))
	(unless (zerop (elt *ball-vector* (+ (* 0 16) (* i 4) j)))
	  (return-from victory-check (elt *ball-vector* (+ (* 0 16) (* i 4) j)))))))
  (dotimes (i 4);;斜め1
    (when (= (elt *ball-vector* (+ (* 0 16) (* 0 4) i))
	     (elt *ball-vector* (+ (* 1 16) (* 1 4) i))
	     (elt *ball-vector* (+ (* 2 16) (* 2 4) i))
	     (elt *ball-vector* (+ (* 3 16) (* 3 4) i)))
      (unless (zerop (elt *ball-vector* (+ (* 0 16) (* 0 4) i)))
	(return-from victory-check (elt *ball-vector* (+ (* 0 16) (* 0 4) i))))))
  (dotimes (i 4);;斜め2
    (when (= (elt *ball-vector* (+ (* 0 16) (* 3 4) i))
	     (elt *ball-vector* (+ (* 1 16) (* 2 4) i))
	     (elt *ball-vector* (+ (* 2 16) (* 1 4) i))
	     (elt *ball-vector* (+ (* 3 16) (* 0 4) i)))
      (unless (zerop (elt *ball-vector* (+ (* 0 16) (* 3 4) i)))
	(return-from victory-check (elt *ball-vector* (+ (* 0 16) (* 3 4) i))))))
  (dotimes (i 4);;斜め3
    (when (= (elt *ball-vector* (+ (* 0 16) (* i 4) 0))
	     (elt *ball-vector* (+ (* 1 16) (* i 4) 1))
	     (elt *ball-vector* (+ (* 2 16) (* i 4) 2))
	     (elt *ball-vector* (+ (* 3 16) (* i 4) 3)))
      (unless (zerop (elt *ball-vector* (+ (* 0 16) (* i 4) 0)))
	(return-from victory-check (elt *ball-vector* (+ (* 0 16) (* i 4) 0))))))
  (dotimes (i 4);;斜め4
    (when (= (elt *ball-vector* (+ (* 0 16) (* i 4) 3))
	     (elt *ball-vector* (+ (* 1 16) (* i 4) 2))
	     (elt *ball-vector* (+ (* 2 16) (* i 4) 1))
	     (elt *ball-vector* (+ (* 3 16) (* i 4) 0)))
      (unless (zerop (elt *ball-vector* (+ (* 0 16) (* i 4) 3)))
	(return-from victory-check (elt *ball-vector* (+ (* 0 16) (* i 4) 3))))))
  (dotimes (i 4);;斜め5
    (when (= (elt *ball-vector* (+ (* i 16) (* 0 4) 0))
	     (elt *ball-vector* (+ (* i 16) (* 1 4) 1))
	     (elt *ball-vector* (+ (* i 16) (* 2 4) 2))
	     (elt *ball-vector* (+ (* i 16) (* 3 4) 3)))
      (unless (zerop (elt *ball-vector* (+ (* i 16) (* 0 4) 0)))
	(return-from victory-check (elt *ball-vector* (+ (* i 16) (* 0 4) 0))))))
  (dotimes (i 4);;斜め6
    (when (= (elt *ball-vector* (+ (* i 16) (* 0 4) 3))
	     (elt *ball-vector* (+ (* i 16) (* 1 4) 2))
	     (elt *ball-vector* (+ (* i 16) (* 2 4) 1))
	     (elt *ball-vector* (+ (* i 16) (* 3 4) 0)))
      (unless (zerop (elt *ball-vector* (+ (* i 16) (* 0 4) 3)))
	(return-from victory-check (elt *ball-vector* (+ (* i 16) (* 0 4) 3))))))
  ;;残りの斜め
  (when (= (elt *ball-vector* (+ (* 0 16) (* 0 4) 0))
	   (elt *ball-vector* (+ (* 1 16) (* 1 4) 1))
	   (elt *ball-vector* (+ (* 2 16) (* 2 4) 2))
	   (elt *ball-vector* (+ (* 3 16) (* 3 4) 3)))
    (unless (zerop (elt *ball-vector* (+ (* 0 16) (* 0 4) 0)))
      (return-from victory-check (elt *ball-vector* (+ (* 0 16) (* 0 4) 0)))))
  (when (= (elt *ball-vector* (+ (* 0 16) (* 0 4) 3))
	   (elt *ball-vector* (+ (* 1 16) (* 1 4) 2))
	   (elt *ball-vector* (+ (* 2 16) (* 2 4) 1))
	   (elt *ball-vector* (+ (* 3 16) (* 3 4) 0)))
    (unless (zerop (elt *ball-vector* (+ (* 0 16) (* 0 4) 3)))
      (return-from victory-check (elt *ball-vector* (+ (* 0 16) (* 0 4) 3)))))
  (when (= (elt *ball-vector* (+ (* 0 16) (* 3 4) 0))
	     (elt *ball-vector* (+ (* 1 16) (* 2 4) 1))
	     (elt *ball-vector* (+ (* 2 16) (* 1 4) 2))
	     (elt *ball-vector* (+ (* 3 16) (* 0 4) 3)))
      (unless (zerop (elt *ball-vector* (+ (* 0 16) (* 3 4) 0)))
	(return-from victory-check (elt *ball-vector* (+ (* 0 16) (* 3 4) 0)))))
  (when (= (elt *ball-vector* (+ (* 0 16) (* 3 4) 3))
	     (elt *ball-vector* (+ (* 1 16) (* 2 4) 2))
	     (elt *ball-vector* (+ (* 2 16) (* 1 4) 1))
	     (elt *ball-vector* (+ (* 3 16) (* 0 4) 0)))
      (unless (zerop (elt *ball-vector* (+ (* 0 16) (* 3 4) 3)))
	(return-from victory-check (elt *ball-vector* (+ (* 0 16) (* 3 4) 3)))))
  nil)

;;armが持ったボールの位置を受信する
;;赤
(setq *armpos-cb-num-red* -10)
(setq *armpos-red* nil)
(setq *armpos-time-red* (ros::time-now))
(dotimes (i 10)
  (push nil *armpos-red*))
(defun armpos-red-cb (msg) ;;過去10コマの平均値をとる ;;lambdaでやると、対話型インタプリタでros::spin-onceした時にnever returnする
  (when (and (< (send msg :point :x) 0) (> (send msg :point :x) 0)) ;;nan
    (return-from armpos-red-cb))
  ;;座標変換
  (send msg :point (send *realsense-camera* :transform-vector (ros::tf-point->pos (send msg :point)))) 
  ;;(send msg :point (send *realsense-camera* :transform-vector (ros::tf-point->pos (send msg :point)) :world))
  (when (< *armpos-cb-num-red* 0) ;;10コマ集まっていないとき
    (setf (elt *armpos-red* (+ *armpos-cb-num-red* 10)) msg)
    (incf *armpos-cb-num-red*)
    (return-from armpos-red-cb))
  (setf (elt *armpos-red* *armpos-cb-num-red*) msg)
  (incf *armpos-cb-num-red*)
  (if (> *armpos-cb-num-red* 9)
      (setq *armpos-cb-num-red* 0))
  (setq *armpos-time-red* (send (elt *armpos-red* *armpos-cb-num-red*) :header :stamp));;計算に影響する最も古いtime
  )
(ros::subscribe "/centroidpublisher_red/output/point" geometry_msgs::PointStamped #'armpos-red-cb)

;;青
(setq *armpos-cb-num-blue* -10)
(setq *armpos-blue* nil)
(setq *armpos-time-blue* (ros::time-now))
(dotimes (i 10)
  (push nil *armpos-blue*))
(defun armpos-blue-cb (msg) ;;過去10コマの平均値をとる ;;lambdaでやると、対話型インタプリタでros::spin-onceした時にnever returnする
  (when (and (< (send msg :point :x) 0) (> (send msg :point :x) 0)) ;;nan
    (return-from armpos-blue-cb))
  ;;座標変換
  (send msg :point (send *realsense-camera* :transform-vector (ros::tf-point->pos (send msg :point)))) 
  (when (< *armpos-cb-num-blue* 0) ;;10コマ集まっていないとき
    (setf (elt *armpos-blue* (+ *armpos-cb-num-blue* 10)) msg)
    (incf *armpos-cb-num-blue*)
    (return-from armpos-blue-cb))
  (setf (elt *armpos-blue* *armpos-cb-num-blue*) msg)
  (incf *armpos-cb-num-blue*)
  (if (> *armpos-cb-num-blue* 9)
      (setq *armpos-cb-num-blue* 0))
  (setq *armpos-time-blue* (send (elt *armpos-blue* *armpos-cb-num-blue*) :header :stamp));;計算に影響する最も古いtime
  )
(ros::subscribe "/centroidpublisher_blue/output/point" geometry_msgs::PointStamped #'armpos-blue-cb)

(defun get-armpos (color)
  (let ((targetlist (if (> color 0)
		       *armpos-red*
		     *armpos-blue*))
	(sum #f(0 0 0)))
  ;;平均をとる
    (dotimes (j 10)
      (setf sum (v+ sum (send (elt targetlist j) :point)))
      )
    (setf sum (scale 0.1 sum))
    sum
    ))
(defun wait-armpos-time (color)
  (let ((ref-time (ros::time-now)))
    (while (ros::time< (if (> color 0) *armpos-time-red* *armpos-time-blue*) ref-time)
      (ros::spin-once)
      (x::window-main-one)
      (ros::sleep))))

;;**************************************************************************:
;;actions.l

;;end-coordsを意味のある位置に
;;(send (send *dxl-armed-turtlebot* :arm :end-coords) :move-to (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :translate (float-vector 117.1 5 -13.5))) :world)
;;(send (send *dxl-armed-turtlebot* :arm :end-coords) :move-to (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :translate (float-vector 107.1 -5 -13.5))) :world)
(send (send *dxl-armed-turtlebot* :arm :end-coords) :move-to (make-cascoords :coords (send (send (send *dxl-armed-turtlebot* :arm :end-coords :parent) :copy-worldcoords) :translate (float-vector 112.1 0 -13.5))) :world)

;;inverse-kinematics。
;;poleの根本のposを入れる。それの150mm以上上に移動する。
;;z座標が低すぎると解けない場合があるので、失敗したら目標z座標を上げていく
(defun pole-inverse-kinematics (target)
  (let (temp-target
	(start-angle-vector (send *dxl-armed-turtlebot* :angle-vector)))
    (send *dxl-armed-turtlebot* :reset-pose)
    (dotimes (i 20)
      (print i)
      ;;必要なら、ここでposに対して補正を入れて、アームのたわみに対応する
      (setq temp-target (v+ target (float-vector -20.0 0.0 (+ (* i 10) 150))))
      ;;yが0以上のとき補正
      ;;(when (> (elt temp-target 1) 0)
        ;;(setq temp-target (v+ temp-target #f(0 20 0))))
      ;;(when (> (elt temp-target 1) 100)
	;;(setq temp-target (v+ temp-target #f(0 10 0))))
      (setq result (send *dxl-armed-turtlebot* :inverse-kinematics (make-cascoords :pos temp-target :rpy (float-vector 0 0 pi)) :rotation-axis :z :rthre (deg2rad 20) :thre 1 :link-list (send *dxl-armed-turtlebot* :link-list (send *dxl-armed-turtlebot* :arm :end-coords :parent)) :revert-if-fail t :dump-command nil))
      (when result
	(print (send *dxl-armed-turtlebot* :arm :end-coords))
	(return-from pole-inverse-kinematics result))
      )
    (send *dxl-armed-turtlebot* :angle-vector start-angle-vector)
    nil
    )
  )

(setq *place-target* 0)
;;place
;;置きたい座標を0~15で指定する。
;;アームを動かし、手を離し、戻る。
(defun place (target)
  ;;inverse-kinametics成功か失敗かで分岐
  (if (pole-inverse-kinematics (send (elt *pole-list* target) :worldpos))
      (progn
	;;(setq *pole-update* nil);;カメラが動いている間は座標を更新しない
	(setq to (send *dxl-armed-turtlebot* :angle-vector))
	(send *dxl-armed-turtlebot* :reset-pose2)
	(send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 3000)
	(send *ri* :wait-interpolation)
	(send *dxl-armed-turtlebot* :reset-pose)
	(send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 3000)
	(send *ri* :wait-interpolation)
	(send *ri* :angle-vector to 2000)
	(send *ri* :wait-interpolation)
	
	;;(return-from place)
	;;new 画像から誤差を修正する
	(wait-armpos-time *my-color*)
	(setq dif (v- (send (elt *pole-list* target) :worldpos) (get-armpos *my-color*)))
	;; (when (and (> target 3) (< target 12))
	;;   (setq dif (v- dif #f(-20.0 10.0 0)))) ;;重心がずれている
	;; (when (< target 4)
	;;   (setq dif (v- dif #f(-10.0 0.0 0)))) ;;重心がずれている
	;; (when (> target 11)
	;;   (setq dif (v- dif #f(-10.0 20.0 0))))	;;重心がずれている
	(setf (elt dif 2) 0.0);;zは不要
	;;(return-from place)
	(when (pole-inverse-kinematics (v+ dif (send (elt *pole-list* target) :worldpos)))
	  (send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 1000)
	  (send *ri* :wait-interpolation)
	  )
	;;ここまで(new)
	(unix::sleep 1)
	(unless (send *ri* :simulation-modep)
	  (send *ri* :stop-grasp)
	  (send *ri* :wait-interpolation)
	  (when (< (elt (send *ri* :state :angle-vector) 6) 0)
	    (send *ri* :stop-grasp)))
	(unix::sleep 1)
	(send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 1000 :gripper-controller)
	(send *ri* :wait-interpolation)
	(send *dxl-armed-turtlebot* :reset-pose)
	(send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 3000)
	(send *ri* :wait-interpolation)
	(send *dxl-armed-turtlebot* :reset-pose2)
	(send *ri* :angle-vector (send *dxl-armed-turtlebot* :angle-vector) 3000)
	(send *ri* :wait-interpolation)
	(default-pose)
	;;(setq *pole-update* t)
	t
	)
    (progn ;;inverse-kinematicsに失敗したらnilを返す
      nil)))

;;placeを上手に使う関数。
;;置きたい座標を0~15で指定する。
;;inverse-kinematicsを解けるように移動する
;;成功すればt
(defun move-and-place (target)
  (wait-board-detector-time)
  (when (and
	 (< (elt (send (elt *pole-list* target) :worldpos) 0) 500)
	 (> (elt (send (elt *pole-list* target) :worldpos) 0) 450))
    ;;動かずに済むに越したことはない
    (when (place target)
      (return-from move-and-place t)))
  (dotimes (i 3)
    ;;x=500前後がちょうどよいので移動してから試す
    (send *ri* :go-pos (/ (- (elt (send (elt *pole-list* target) :worldpos) 0) 500) 1000) 0)
    (send *ri* :wait-interpolation)
    (unix::sleep 1)
    (wait-board-detector-time)
    (when (place target)
      (return-from move-and-place t)))
  ;;エラーを出すか(要検討)
  nil)

;;把持
(defun start-grasp ()
  ;;(send *ri* :start-grasp :gain 0.5));;gainがdefaultでは0.5 0.5で十分
  (send *ri* :start-grasp))
  
;;把持中かどうか
(defun is-grasp-succeed ()
  ;;完全に閉じてる->ダメ
  (when (< (elt (send *ri* :state :potentio-vector) 6) -25)
    (return-from is-grasp-succeed nil))
  ;;もう1回掴む
  (send *ri* :start-grasp)
  ;;完全に閉じてる->ダメ
  (when (< (elt (send *ri* :state :potentio-vector) 6) -25)
    (return-from is-grasp-succeed nil))
  t)
;;  (when (< (- (elt (send *ri* :actual-vector :gripper-controller) 0) (elt (send *ri* :refeference-vector :gripper-controller) 0)) 3)
;;    (return-from is-grasp-succeed nil))
; ; t)


;;***************************************************************************
;;sensors.l
(setq *button1* nil)
(setq *button2* nil)
(setq *button3* nil)
(defun get-button (num)
  (case num
    (1 (prog1 *button1*
	 (setq *button1* nil)))
    (2 (prog1 *button2*
	 (setq *button2* nil)))
    (3 (prog1 *button3*
	 (setq *button3* nil)))
    )
  )
(defun button-cb (msg)
  (case (send msg :data)
    (1 (setq *button1* t) (print *button1*))
    (2 (setq *button2* t))
    (3 (setq *button3* t))
    )
  )
(ros::subscribe "/main/button" std_msgs::Int32 #'button-cb)


;;**************************************************************************
;;states.l

;;相手の手番完了まで待機する
;;ボタン1が押されると待機が解け、盤の状況の認識に移る
;;debug用に端末から相手の置いた位置を表すtopicを手動で送り、板の状況の認識を飛ばして勝利判定に移ることもできる
;;(debug用に端末から現在の盤の状況を表すtopicを手動で送り、盤の状況の内部状態を一新することもできる)(未実装)
(defun state-wait ()
  (ros::ros-info "waiting for opponent-action")
  (setq *ball-state-teach* t);;rostopic pub -1 /main/board_state_teach std_msgs/Int32 63
  (setq *ball-state-renewal* t)
  (while (ros::ok)
    (ros::spin-once)
    (when *ball-state-renewal-received*
      (setq *ball-state-renewal-received* nil)
      (wait-board-detector-time)
      (draw-balls)
      (setq *ball-state-teach* nil)
      (setq *ball-state-renewal* nil)
      (setq *state* :victory-check-opponent)
      (return-from state-wait)
      )
    (when *ball-state-teach-received*
      (setq *ball-state-teach-received* nil)
      (setq *ball-state-teach* nil)
      (setq *ball-state-renewal* nil)
      (wait-board-detector-time)
      (draw-balls)
      (setq *state* :check-opponent-action)
      (return-from state-wait)
      )
    (when (or (v= (send *ri* :state :button-vector) #f(1 0 0));;ボタン1が押されたら
	      (get-button 1));;rostopic pub -1 /main/button std_msgs/Int32 1
      (send *ri* :publish-sound :button)
      (setq *state* :recog-opponent-action)
      (setq *ball-state-teach* nil)
      (setq *ball-state-renewal* nil)
      (return-from state-wait)
      )
    (x::window-main-one)
    (ros::sleep)
    )
  )

;;ボードの状態を取得する。
(defun state-recog-opponent-action ()
  (ros::ros-info "recog-opponent-action")
  ;;(let ((ref-time (ros::time-now)))
  (update-ball-state)
  (ros::ros-info "recog-opponent-action1")
  (wait-board-detector-time)
  (ros::ros-info "recog-opponent-action2")
  (draw-balls)
  (setq *state* :check-opponent-action)
  (return-from state-recog-opponent-action)
  )

;;変化を評価する
(defun state-check-opponent-action ()
  (ros::ros-info "check-opponent-action")
  (if (check-ball-state)
      ;;問題ない場合
      (progn
	(setq *state* :victory-check-opponent)
	(return-from state-check-opponent-action))
    ;;問題ある場合
    (progn
      (ros::ros-info "error cannot recognize opponent-action")
      (setq *state* :opponent-action-error)
      (return-from state-check-opponent-action))
    )
  )

;;対戦相手の置いた場所の認識がおかしい場合
(defun state-opponent-action-error ()
  (ros::ros-info "opponent-action-error")
  (send *ri* :publish-sound :error)
  (while (ros::ok)
    (ros::spin-once)
    (when (or (v= (send *ri* :state :button-vector) #f(0 3 0));;ボタン3が押されたら
	      (get-button 3));;rostopic pub -1 /main/button std_msgs/Int32 3
      (send *ri* :publish-sound :button)
      (ball-state-undo);;巻き戻し
      (draw-balls)
      (setq *state* :wait)
      (return-from state-opponent-action-error))
    (x::window-main-one)
    (ros::sleep)
    )
  )

;;勝利判定
(defun state-victory-check-opponent ()
  (ros::ros-info "victory-check-opponent")
  (case (victory-check)
    (*my-color*
     (setq *state* :win)
     (return-from state-victory-check-opponent))
    ((- *my-color*)
     (setq *state* :lose)
     (return-from state-victory-check-opponent))
    (nil
     (setq *state* :think)
     (return-from state-victory-check-opponent))
    )
  )

;;次の一手を考える *changepos* *ball-vector*を利用
(setq *t* (instance ros::simple-action-client :init "/think" jisyupro::thinkAction))
(defun state-think ()
  (ros::ros-info "state-think")
  (let ((goal (instance jisyupro::thinkActiongoal :init)))
    (send goal :goal :board :int8s *ball-vector*)
    (send goal :goal :board :header :stamp (ros::time-now))
    (send *t* :send-goal goal)
    (send *t* :wait-for-result)
    (setq *place-target* (send (send *t* :get-result) :newpos)))
  (setq *state* :wait-for-ball)
  (return-from state-think)
  )

;;ボールをハンドに置いてくれるのを待つ
;;ボタン1で、待機終了
;;ボタン2で、ヒトが補助してボールを代わりに置いてあげたことにできる
;;ボタン3で、認識をundo
(defun state-wait-for-ball ()
  (send *ri* :publish-sound :recharge)
  (ros::ros-info "wait-for-ball")
  (unless (send *ri* :simulation-modep)
      (send *ri* :stop-grasp))
  (while (ros::ok)
    (ros::spin-once)
    (when (or (v= (send *ri* :state :button-vector) #f(0 0 1));;ボタン3が押されたら
	      (get-button 3));;rostopic pub -1 /main/button std_msgs/Int32 3
      (send *ri* :publish-sound :button)
      (ball-state-undo);;巻き戻し
      (draw-balls)
      (setq *state* :wait)
      (return-from state-wait-for-ball)
      )
    (when (or (v= (send *ri* :state :button-vector) #f(0 1 0));;ボタン2が押されたら
	      (get-button 2));;rostopic pub -1 /main/button std_msgs/Int32 2
      (send *ri* :publish-sound :button)
      (ball-state-teach *place-target* *my-color*)
      (wait-board-detector-time)
      (draw-balls)
      (setq *state* :check-my-action)
      (return-from state-wait-for-ball))
    (when (or (v= (send *ri* :state :button-vector) #f(1 0 0));;ボタン1が押されたら
	      (get-button 1));;rostopic pub -1 /main/button std_msgs/Int32 1
      (send *ri* :publish-sound :button)
      (setq *state* :start-grasp)
      (return-from state-wait-for-ball))
    (x::window-main-one)
    (ros::sleep)
    )
  )

(defun state-start-grasp ()
  (ros::ros-info "start-grasp")
  (unless (send *ri* :simulation-modep)
    (start-grasp)
    (unless (is-grasp-succeed)
      (send *ri* :stop-grasp)
      (setq *state* :wait-for-ball)
      (return-from state-start-grasp))
    )
  (setq *state* :put-ball)
  (return-from state-start-grasp)
  )

(defun state-put-ball ()
  (ros::ros-info "put-ball")
  (wait-board-detector-time)
  (if (move-and-place (floor (/ *place-target* 4)))
      ;;置く行動そのものは成功
      (progn
	(setq *state* :recog-my-action)
	(return-from state-put-ball))
    ;;inverse-kinematicsそのものに失敗
    (progn
      (ros::ros-info "inverse-kinematics-failed: ~A" *place-target*)
      (send *ri* :publish-sound :error)
      (setq *state* :wait-for-ball)
      (return-from state-put-ball))
    )
  )

;;ボードの状態を取得する。
(defun state-recog-my-action ()
  (ros::ros-info "recog-my-action")
  (let ((ref-time (ros::time-now)))
    (update-ball-state)
    (wait-board-detector-time ref-time))
  (draw-balls)
  (setq *state* :check-my-action)
  (return-from state-recog-my-action)
  )

;;
(defun state-check-my-action ()
  (ros::ros-info "check-my-action")
  (if (check-ball-state)
      ;;問題ない場合
      (if (= *changepos* *place-target*)
	  (progn
	    (setq *state* :victory-check-me)
	    (return-from state-check-my-action))
	;;間違えた場所に置いた
	(progn
	  (ros::ros-info "error I put wrong position")
	  (send *ri* :publish-sound :error)
	  (ball-state-undo);;巻き戻し
	  (draw-balls)
	  (setq *state* :wait-for-ball)
	  (return-from state-check-my-action)))
    ;;問題ある場合
    (progn
      (ros::ros-info "error cannot recognize my-action")
      (send *ri* :publish-sound :error)
      (ball-state-undo);;巻き戻し
      (draw-balls)
      (setq *state* :wait-for-ball)
      (return-from state-check-my-action))
    )
  )

(defun state-victory-check-me ()
  (ros::ros-info "victory-check-me")
  (case (victory-check)
    (*my-color*
     (setq *state* :win)
     (return-from state-victory-check-me))
    ((- *my-color*)
     (setq *state* :lose)
     (return-from state-victory-check-me))
    (nil
     (setq *state* :wait)
     (return-from state-victory-check-me))
    )
  )

(defun state-win ()
  (send *ri* :publish-sound :on)
  (ros::ros-info "I win !")
  (while (ros::ok)
    (ros::spin-once)
    (when (or (v= (send *ri* :state :button-vector) #f(0 0 1));;ボタン3が押されたら
	      (get-button 3));;rostopic pub -1 /main/button std_msgs/Int32 3
      (send *ri* :publish-sound :button)
      (ball-state-undo);;巻き戻し
      (draw-balls)
      (setq *state* :wait)
      (return-from state-win)
      )
    (ros::sleep)
    )
  )

(defun state-lose ()
  (send *ri* :publish-sound :of)
  (ros::ros-info "You win !")
  (while (ros::ok)
    (ros::spin-once)
    (when (or (v= (send *ri* :state :button-vector) #f(0 0 1));;ボタン3が押されたら
	      (get-button 3));;rostopic pub -1 /main/button std_msgs/Int32 3
      (send *ri* :publish-sound :button)
      (ball-state-undo);;巻き戻し
      (draw-balls)
      (setq *state* :wait)
      (return-from state-lose)
      )
    (ros::sleep)
    )
  )

;;*************************************************************************
;;main.l

;;後攻の場合
(setq *state* :wait)
(default-pose)

(defun mainloop ()
  (ros::rate 10)
  (while (ros::ok)
    (case *state*
      (:wait (state-wait))
      (:recog-opponent-action (state-recog-opponent-action))
      (:check-opponent-action (state-check-opponent-action))
      (:opponent-action-error (state-opponent-action-error))
      (:victory-check-opponent (state-victory-check-opponent))
      (:think (state-think))
      (:wait-for-ball (state-wait-for-ball))
      (:start-grasp (state-start-grasp))
      (:put-ball (state-put-ball))
      (:recog-my-action (state-recog-my-action))
      (:check-my-action (state-check-my-action))
      (:victory-check-me (state-victory-check-me))
      (:win (state-win))
      (:lose (state-lose))
      (t (ros::ros-info "unknown-state")
	 (return))
      )
    )
  )
