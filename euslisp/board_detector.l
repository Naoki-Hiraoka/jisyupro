#!/usr/bin/env roseus

(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "jisyupro")
(ros::load-ros-manifest "std_msgs")
(when (require "package://dxl_armed_turtlebot/euslisp/dxl-armed-turtlebot-interface.l")
  (dxl-armed-turtlebot-init))

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
(setq *c* (instance ros::simple-action-client :init "board_state_recorder" jisyupro::board_state_recorderAction))
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
  (send *c* :wait-for-result)
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
  (send *dxl-armed-turtlebot* :tuckarm-pose2)
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
    
