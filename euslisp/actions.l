#!/usr/bin/env/roseus

(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "jisyupro")
(when (require "package://dxl_armed_turtlebot/euslisp/dxl-armed-turtlebot-interface.l")
  (dxl-armed-turtlebot-init))
(require "package://jisyupro/euslisp/board_detector.l")

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
