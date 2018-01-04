#!/usr/bin/env roseus

(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "jisyupro")
(when (require "package://dxl_armed_turtlebot/euslisp/dxl-armed-turtlebot-interface.l")
  (dxl-armed-turtlebot-init))
(require "package://jisyupro/euslisp/board_detector.l")
(require "package://jisyupro/euslisp/actions.l")
(require "package://jisyupro/euslisp/sensors.l")

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
(setq *ct* (instance ros::simple-action-client :init "/think" jisyupro::thinkAction))
(defun state-think ()
  (ros::ros-info "state-think")
  (let ((goal (instance jisyupro::thinkActiongoal :init)))
    (send goal :goal :board :int8s *ball-vector*)
    (send goal :goal :board :header :stamp (ros::time-now))
    (send *ct* :send-goal goal)
    (send *ct* :wait-for-result)
    (setq *place-target* (send (send *ct* :get-result) :newpos)))
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
